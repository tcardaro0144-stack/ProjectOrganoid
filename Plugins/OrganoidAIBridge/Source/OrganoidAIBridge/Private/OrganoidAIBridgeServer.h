#pragma once

#include "CoreMinimal.h"
#include "HttpResultCallback.h"
#include "HttpRouteHandle.h"
#include "Templates/SharedPointer.h"

class IHttpRouter;
class FOrganoidAIBridgeLogSink;
struct FHttpServerRequest;

class FOrganoidAIBridgeServer : public TSharedFromThis<FOrganoidAIBridgeServer>
{
public:
	FOrganoidAIBridgeServer();
	~FOrganoidAIBridgeServer();

	bool Start();
	void Stop();

private:
	bool HandleHealth(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleCommand(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool IsLoopbackPeer(const FHttpServerRequest& Request) const;
	void ReplyJson(const FHttpResultCallback& OnComplete, int32 HttpCode, const FString& Json);

	TSharedPtr<IHttpRouter> Router;
	FHttpRouteHandle HealthRoute;
	FHttpRouteHandle CommandRoute;
	TUniquePtr<FOrganoidAIBridgeLogSink> LogSink;
	uint32 Port = 8732;
	bool bReadOnly = true;
	FString AuthToken;
};
