#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace OrganoidAIBridgeJson
{
	inline const TCHAR* BridgeVersion()
	{
		return TEXT("0.5.9");
	}

	inline TSharedPtr<FJsonObject> ParseObject(const FString& Text)
	{
		TSharedPtr<FJsonObject> Object;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		FJsonSerializer::Deserialize(Reader, Object);
		return Object;
	}

	inline FString ToString(const TSharedRef<FJsonObject>& Object)
	{
		FString Out;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Object, Writer);
		return Out;
	}

	inline TSharedRef<FJsonObject> Ok(const TSharedRef<FJsonObject>& Data)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), true);
		Root->SetObjectField(TEXT("data"), Data);
		return Root;
	}

	inline TSharedRef<FJsonObject> Fail(const FString& Code, const FString& Message)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), false);
		Root->SetStringField(TEXT("error_code"), Code);
		Root->SetStringField(TEXT("error"), Message);
		return Root;
	}

	inline TArray<TSharedPtr<FJsonValue>> Vec(const FVector& V)
	{
		TArray<TSharedPtr<FJsonValue>> Arr;
		Arr.Add(MakeShared<FJsonValueNumber>(V.X));
		Arr.Add(MakeShared<FJsonValueNumber>(V.Y));
		Arr.Add(MakeShared<FJsonValueNumber>(V.Z));
		return Arr;
	}

	inline TArray<TSharedPtr<FJsonValue>> Rot(const FRotator& R)
	{
		TArray<TSharedPtr<FJsonValue>> Arr;
		Arr.Add(MakeShared<FJsonValueNumber>(R.Pitch));
		Arr.Add(MakeShared<FJsonValueNumber>(R.Yaw));
		Arr.Add(MakeShared<FJsonValueNumber>(R.Roll));
		return Arr;
	}

	inline FString EnumName(const UEnum* Enum, int64 Value)
	{
		if (!Enum)
		{
			return FString::FromInt(static_cast<int32>(Value));
		}
		return Enum->GetNameStringByValue(Value);
	}

	inline FString GetString(const TSharedPtr<FJsonObject>& Object, const FString& Field, const FString& Default = FString())
	{
		if (!Object.IsValid())
		{
			return Default;
		}
		FString Value;
		if (Object->TryGetStringField(Field, Value))
		{
			return Value;
		}
		return Default;
	}

	inline bool GetBool(const TSharedPtr<FJsonObject>& Object, const FString& Field, bool Default = false)
	{
		if (!Object.IsValid())
		{
			return Default;
		}
		bool Value = Default;
		if (Object->TryGetBoolField(Field, Value))
		{
			return Value;
		}
		return Default;
	}

	inline int32 GetInt(const TSharedPtr<FJsonObject>& Object, const FString& Field, int32 Default = 0)
	{
		if (!Object.IsValid())
		{
			return Default;
		}
		int32 Value = Default;
		if (Object->TryGetNumberField(Field, Value))
		{
			return Value;
		}
		return Default;
	}

	inline double GetNumber(const TSharedPtr<FJsonObject>& Object, const FString& Field, double Default = 0.0)
	{
		if (!Object.IsValid())
		{
			return Default;
		}
		double Value = Default;
		if (Object->TryGetNumberField(Field, Value))
		{
			return Value;
		}
		return Default;
	}

	inline bool GetVector(const TSharedPtr<FJsonObject>& Object, const FString& Field, FVector& Out)
	{
		if (!Object.IsValid())
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (!Object->TryGetArrayField(Field, Arr) || !Arr || Arr->Num() < 3)
		{
			return false;
		}
		Out.X = (*Arr)[0]->AsNumber();
		Out.Y = (*Arr)[1]->AsNumber();
		Out.Z = (*Arr)[2]->AsNumber();
		return true;
	}
}
