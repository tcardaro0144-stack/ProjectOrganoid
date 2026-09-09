#pragma once

#include "CoreMinimal.h"
#include "Misc/OutputDevice.h"

class FOrganoidAIBridgeLogSink : public FOutputDevice
{
public:
	explicit FOrganoidAIBridgeLogSink(int32 InMaxLines)
		: MaxLines(FMath::Max(100, InMaxLines))
	{
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!V)
		{
			return;
		}

		const FString Line = FString::Printf(
			TEXT("%s: %s"),
			*Category.ToString(),
			V);

		FScopeLock Lock(&Mutex);
		Lines.Add(Line);
		if (Lines.Num() > MaxLines)
		{
			const int32 RemoveCount = Lines.Num() - MaxLines;
			Lines.RemoveAt(0, RemoveCount, EAllowShrinking::No);
		}
	}

	TArray<FString> CopyFiltered(const FString& Filter, int32 MaxReturn) const
	{
		FScopeLock Lock(&Mutex);
		TArray<FString> Out;
		const int32 Limit = MaxReturn > 0 ? MaxReturn : 200;
		for (int32 Index = Lines.Num() - 1; Index >= 0 && Out.Num() < Limit; --Index)
		{
			if (Filter.IsEmpty() || Lines[Index].Contains(Filter, ESearchCase::IgnoreCase))
			{
				Out.Insert(Lines[Index], 0);
			}
		}
		return Out;
	}

private:
	mutable FCriticalSection Mutex;
	TArray<FString> Lines;
	int32 MaxLines = 2000;
};
