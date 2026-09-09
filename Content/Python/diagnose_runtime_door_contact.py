# ProjectOrganoid — READ-ONLY PIE contact dump (Section 17).
# Identifies what the possessed pawn capsule actually hits at the stop.
# Uses GetPawn + Pawn-profile capsule sweep. Does not spawn, move, save,
# compile, or touch maps / Blueprints / S1–S16 / door collision.
#
# Run WHILE PIE is playing, standing at the invisible stop:
#   py "C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Content\Python\diagnose_runtime_door_contact.py"

import unreal

TAG = "[S17 CONTACT] "


def _log(msg):
    unreal.log(TAG + msg)


def _enum_name(value):
    text = str(value)
    if "." in text:
        return text.rsplit(".", 1)[-1]
    return text


def _safe(obj, name, *args):
    method = getattr(obj, name, None) if obj is not None else None
    if not callable(method):
        return None
    try:
        return method(*args)
    except Exception as exc:
        return "ERR:%s" % exc


def _fmt_vec(vec):
    if vec is None:
        return "None"
    return "(%.2f, %.2f, %.2f)" % (vec.x, vec.y, vec.z)


def _comp_name(comp):
    if comp is None:
        return "None"
    return "%s" % (_safe(comp, "get_name") or comp)


def _actor_name(actor):
    if actor is None:
        return "None"
    label = _safe(actor, "get_actor_label")
    name = _safe(actor, "get_name")
    cls = _safe(actor, "get_class")
    cls_name = _safe(cls, "get_name") if cls else None
    return "label=%s name=%s class=%s" % (label or "None", name or "None", cls_name or "None")


def _hit_fields(hit):
    if hit is None:
        return None
    actor = _safe(hit, "get_actor")
    if actor is None:
        actor = getattr(hit, "actor", None)
        actor = actor.get() if hasattr(actor, "get") else actor
    comp = getattr(hit, "component", None)
    if hasattr(comp, "get"):
        comp = comp.get()
    if comp is None:
        comp = _safe(hit, "get_component")
    impact = getattr(hit, "impact_point", None)
    normal = getattr(hit, "impact_normal", None)
    loc = getattr(hit, "location", None)
    dist = getattr(hit, "distance", None)
    time = getattr(hit, "time", None)
    blocking = getattr(hit, "blocking_hit", None)
    return actor, comp, impact, normal, loc, dist, time, blocking


def _log_hit(prefix, hit):
    parsed = _hit_fields(hit)
    if parsed is None:
        _log("%s <no hit>" % prefix)
        return
    actor, comp, impact, normal, loc, dist, time, blocking = parsed
    _log("%s blocking=%s time=%s dist=%s" % (prefix, blocking, time, dist))
    _log("%s actor %s" % (prefix, _actor_name(actor)))
    _log("%s component=%s" % (prefix, _comp_name(comp)))
    if comp is not None:
        _log("%s profile=%s enabled=%s pawn=%s" % (
            prefix,
            _safe(comp, "get_collision_profile_name"),
            _enum_name(_safe(comp, "get_collision_enabled")),
            _enum_name(_safe(comp, "get_collision_response_to_channel", unreal.CollisionChannel.PAWN)),
        ))
    _log("%s impact=%s normal=%s sweep_loc=%s" % (
        prefix, _fmt_vec(impact), _fmt_vec(normal), _fmt_vec(loc)
    ))


def _call_trace(fn, **kwargs):
    """UE Python sometimes returns out-hits, sometimes fills a list argument."""
    hits = []
    try:
        result = fn(**kwargs)
    except TypeError:
        kwargs["out_hits"] = hits
        result = fn(**kwargs)
        return result, hits
    if isinstance(result, (list, tuple)):
        if len(result) == 2 and isinstance(result[1], (list, tuple)):
            return result[0], list(result[1])
        if result and hasattr(result[0], "blocking_hit"):
            return True, list(result)
    if hasattr(result, "blocking_hit"):
        return True, [result]
    return result, hits


world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if world is None:
    _log("NO PIE WORLD — start Play, stand at the stop, run this again.")
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if pawn is None:
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        pawn = pc.get_pawn() if pc else None
    if pawn is None:
        _log("NO POSSESSED PAWN")
    else:
        loc = pawn.get_actor_location()
        capsule = pawn.capsule_component if hasattr(pawn, "capsule_component") else None
        if capsule is None:
            capsule = _safe(pawn, "get_capsule_component")
        radius = float(_safe(capsule, "get_unscaled_capsule_radius") or 42.0)
        half = float(_safe(capsule, "get_unscaled_capsule_half_height") or 96.0)
        plus_x = loc.x + radius
        _log("pawn=%s" % pawn.get_name())
        _log("loc=%s radius=%.2f half=%.2f" % (_fmt_vec(loc), radius, half))
        _log("capsule_+X_surface=%.2f  implied_west_face_if_blocked=%.2f" % (plus_x, plus_x))
        _log("capsule_Y_range=%.2f..%.2f" % (loc.y - radius, loc.y + radius))

        ignore = [pawn]
        start = unreal.Vector(loc.x - 20.0, loc.y, loc.z)
        end = unreal.Vector(loc.x + 80.0, loc.y, loc.z)
        line_end = unreal.Vector(loc.x + 120.0, loc.y, loc.z)

        _log("--- capsule sweep +X profile=Pawn from X-20 to X+80 ---")
        did, hits = _call_trace(
            unreal.SystemLibrary.capsule_trace_multi_by_profile,
            world_context_object=world,
            start=start,
            end=end,
            radius=radius,
            half_height=half,
            profile_name="Pawn",
            trace_complex=False,
            actors_to_ignore=ignore,
            draw_debug_type=unreal.DrawDebugTrace.FOR_DURATION,
            ignore_self=True,
            draw_time=8.0,
        )
        _log("sweep_returned=%s hit_count=%d" % (did, len(hits)))
        if not hits:
            _log("NO SWEEP HITS")
        for i, hit in enumerate(hits):
            _log_hit("sweep[%d]" % i, hit)

        _log("--- line trace +X from capsule center to X+120 profile=Pawn ---")
        did_line, line_hits = _call_trace(
            unreal.SystemLibrary.line_trace_multi_by_profile,
            world_context_object=world,
            start=loc,
            end=line_end,
            profile_name="Pawn",
            trace_complex=False,
            actors_to_ignore=ignore,
            draw_debug_type=unreal.DrawDebugTrace.FOR_DURATION,
            ignore_self=True,
            draw_time=8.0,
        )
        _log("line_returned=%s hit_count=%d" % (did_line, len(line_hits)))
        if not line_hits:
            _log("NO LINE HITS")
        for i, hit in enumerate(line_hits):
            _log_hit("line[%d]" % i, hit)

        _log("--- BP_AdminAccessDoor live primitives ---")
        actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        doors = []
        for actor in actors:
            blob = "%s %s" % (
                _safe(_safe(actor, "get_class"), "get_name"),
                _safe(_safe(actor, "get_class"), "get_path_name"),
            )
            if "AdminAccessDoor" in blob:
                doors.append(actor)
        _log("door_instance_count=%d" % len(doors))
        for door in doors:
            dloc = door.get_actor_location()
            _log("door %s loc=%s" % (_actor_name(door), _fmt_vec(dloc)))
            comps = door.get_components_by_class(unreal.PrimitiveComponent)
            for comp in comps:
                bounds = getattr(comp, "bounds", None)
                origin = getattr(bounds, "origin", None) if bounds else None
                extent = getattr(bounds, "box_extent", None) if bounds else None
                west = None
                y0 = y1 = None
                if origin is not None and extent is not None:
                    west = origin.x - abs(extent.x)
                    y0 = origin.y - abs(extent.y)
                    y1 = origin.y + abs(extent.y)
                rel = _safe(comp, "get_relative_location")
                covers_y = None
                if y0 is not None:
                    covers_y = (y0 <= loc.y + radius) and (y1 >= loc.y - radius)
                pawn_resp = _enum_name(_safe(comp, "get_collision_response_to_channel", unreal.CollisionChannel.PAWN))
                _log(
                    "  %s profile=%s enabled=%s pawn=%s rel=%s origin=%s extent=%s west_x=%s y=%s..%s covers_pawn_y=%s"
                    % (
                        _comp_name(comp),
                        _safe(comp, "get_collision_profile_name"),
                        _enum_name(_safe(comp, "get_collision_enabled")),
                        pawn_resp,
                        _fmt_vec(rel),
                        _fmt_vec(origin),
                        _fmt_vec(extent),
                        ("%.2f" % west) if west is not None else "None",
                        ("%.2f" % y0) if y0 is not None else "None",
                        ("%.2f" % y1) if y1 is not None else "None",
                        covers_y,
                    )
                )
        _log("DONE")
