# ProjectOrganoid — generate and import looping facility beds.
# Called from build_epitope_rooms.py so partitions always have real sounds.

import math
import os
import struct
import wave

try:
    import unreal
except ImportError:
    unreal = None


AUDIO_DIR = "/Game/Audio/Ambient"
BEDS = {
    "SW_FacilityBed": {"kind": "brown", "seconds": 4.0, "gain": 0.22},
    "SW_TensionBed": {"kind": "rumble", "seconds": 4.0, "gain": 0.18},
    # UV-C/decon telegraph: dark ventilation + faint electrical grain.
    # No 2400 Hz sine, no alarm pulse. 8 s loop with cosine crossfade.
    "SW_HazardHiss": {"kind": "hiss", "seconds": 8.0, "gain": 0.20, "fade": 0.5},
    "SW_AlarmPulse": {"kind": "pulse", "seconds": 1.2, "gain": 0.12},
}


def _source_dir():
    if unreal:
        return os.path.join(unreal.Paths.project_saved_dir(), "AudioGen")
    here = os.path.dirname(os.path.abspath(__file__))
    project = os.path.abspath(os.path.join(here, "..", ".."))
    return os.path.join(project, "Saved", "AudioGen")


def report(message):
    unreal.log_warning(f"[AUDIO] {message}")


def _clamp_sample(value):
    return max(-32767, min(32767, int(value)))


def _write_wav(path, samples, sample_rate=22050):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "w") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(b"".join(struct.pack("<h", sample) for sample in samples))


def _generate_samples(kind, seconds, gain, sample_rate=22050, fade_seconds=0.0):
    count = int(sample_rate * seconds)
    fade_n = int(sample_rate * fade_seconds) if fade_seconds > 0.0 else 0
    total = count + fade_n
    raw = []
    brown = 0.0
    rumble = 0.0
    seed = 7919 if kind == "brown" else 104729

    for i in range(total):
        seed = (1103515245 * seed + 12345) & 0x7FFFFFFF
        noise = (seed / 0x40000000) - 1.0
        t = i / float(sample_rate)

        if kind == "brown":
            brown = brown * 0.985 + noise * 0.015
            value = brown
        elif kind == "rumble":
            rumble = rumble * 0.992 + noise * 0.008
            value = rumble + 0.12 * math.sin(2.0 * math.pi * 48.0 * t)
        elif kind == "hiss":
            # Dark duct air + sparse sterilized grain. No tonal beep/whistle.
            # Modulators complete integer cycles over the 8 s loop.
            brown = brown * 0.985 + noise * 0.015
            air = brown
            hvac = 0.03 * math.sin(2.0 * math.pi * 40.0 * t)
            wobble = 1.0 + 0.06 * math.sin(2.0 * math.pi * 0.125 * t)
            hp = noise - rumble
            rumble = rumble * 0.94 + noise * 0.06
            grain_env = 0.5 + 0.5 * math.sin(2.0 * math.pi * 0.25 * t)
            grain = hp * 0.022 * grain_env
            value = (air + hvac + grain) * wobble
        else:
            envelope = max(0.0, 1.0 - (t / 0.18)) if t < 0.18 else 0.0
            if 0.55 <= t <= 0.70:
                envelope = max(envelope, 1.0 - abs(t - 0.62) / 0.08)
            value = envelope * math.sin(2.0 * math.pi * 880.0 * t)

        raw.append(value * gain)

    if kind == "hiss":
        # Strip brown-noise wander so the loop end isn't parked on a DC plateau.
        prev_x = 0.0
        prev_y = 0.0
        blocked = []
        for x in raw:
            y = x - prev_x + 0.995 * prev_y
            blocked.append(y)
            prev_x = x
            prev_y = y
        raw = blocked

    if fade_n > 0:
        for i in range(fade_n):
            fade_in = 0.5 - 0.5 * math.cos(math.pi * i / float(fade_n))
            raw[i] = raw[count + i] * (1.0 - fade_in) + raw[i] * fade_in
        raw = raw[:count]

    mean = sum(raw) / float(len(raw)) if raw else 0.0
    raw = [value - mean for value in raw]
    return [_clamp_sample(value * 32767.0) for value in raw]


def write_bed_wav(name, dest_dir=None):
    spec = BEDS[name]
    dest_dir = dest_dir or _source_dir()
    wav_path = os.path.join(dest_dir, f"{name}.wav")
    samples = _generate_samples(
        spec["kind"],
        spec["seconds"],
        spec["gain"],
        fade_seconds=spec.get("fade", 0.0),
    )
    _write_wav(wav_path, samples)
    return wav_path


def import_wav(src_path, dest_name):
    if not unreal:
        raise RuntimeError("import_wav requires Unreal Python")
    if not unreal.EditorAssetLibrary.does_directory_exist(AUDIO_DIR):
        unreal.EditorAssetLibrary.make_directory(AUDIO_DIR)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src_path)
    task.set_editor_property("destination_path", AUDIO_DIR)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = f"{AUDIO_DIR}/{dest_name}"
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        report(f"import failed: {asset_path}")
        return None

    for prop in ("looping", "bLooping", "b_looping"):
        try:
            asset.set_editor_property(prop, True)
            break
        except Exception:
            continue

    unreal.EditorAssetLibrary.save_asset(asset_path)
    return asset


def ensure_ambient_audio():
    report("=== Facility ambient beds ===")
    imported = {}
    for name, spec in BEDS.items():
        wav_path = write_bed_wav(name)
        asset = import_wav(wav_path, name)
        if asset:
            imported[name] = f"{AUDIO_DIR}/{name}.{name}"
            report(f"ready {imported[name]}")
    report(f"=== Audio complete: {len(imported)} / {len(BEDS)} ===")
    return imported


def rebuild_hazard_hiss_only():
    report("=== Rebuild SW_HazardHiss only ===")
    spec = BEDS["SW_HazardHiss"]
    wav_path = write_bed_wav("SW_HazardHiss")
    asset = import_wav(wav_path, "SW_HazardHiss")
    if not asset:
        report("SW_HazardHiss import failed")
        return None
    report(
        f"ready {AUDIO_DIR}/SW_HazardHiss.SW_HazardHiss "
        f"kind={spec['kind']} seconds={spec['seconds']} gain={spec['gain']} fade={spec.get('fade', 0.0)}"
    )
    return f"{AUDIO_DIR}/SW_HazardHiss.SW_HazardHiss"


if __name__ == "__main__":
    if unreal:
        rebuild_hazard_hiss_only()
    else:
        path = write_bed_wav("SW_HazardHiss")
        print(path)
