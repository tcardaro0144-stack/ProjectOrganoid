#pragma once

#include "Misc/OutputDevice.h"

class FOrganoidPlaytestLogSink : public FOutputDevice
{
public:
	TArray<FString> Lines;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	int32 Cursor = 0;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		const FString CategoryName = Category.ToString();
		const FString Line = FString::Printf(TEXT("%s: %s"), *CategoryName, V ? V : TEXT(""));
		Lines.Add(Line);

		const bool bBlueprintish =
			CategoryName.Contains(TEXT("Blueprint"))
			|| CategoryName.Contains(TEXT("LogScript"))
			|| Line.Contains(TEXT("Blueprint Runtime Error"))
			|| Line.Contains(TEXT("Script Stack"));

		if (Verbosity <= ELogVerbosity::Error)
		{
			if (bBlueprintish || CategoryName.Contains(TEXT("Python")))
			{
				Errors.Add(Line);
			}
		}
		else if (Verbosity == ELogVerbosity::Warning && bBlueprintish)
		{
			Warnings.Add(Line);
		}
	}

	void MarkCursor()
	{
		Cursor = Lines.Num();
	}

	TArray<FString> LinesSinceCursor() const
	{
		TArray<FString> Out;
		for (int32 Index = Cursor; Index < Lines.Num(); ++Index)
		{
			Out.Add(Lines[Index]);
		}
		return Out;
	}

	bool ContainsSinceCursor(const FString& Needle) const
	{
		for (const FString& Line : LinesSinceCursor())
		{
			if (Line.Contains(Needle))
			{
				return true;
			}
		}
		return false;
	}
};
