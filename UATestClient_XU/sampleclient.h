#ifndef SAMPLECLIENT_H
#define SAMPLECLIENT_H

#include "uabase.h"
#include "uaclientsdk.h"
#include "uaargument.h"
#include <fstream>

class Configuration;

using namespace UaClientSdk;

class SampleClient : public UaSessionCallback
{
    UA_DISABLE_COPY(SampleClient);
public:
    SampleClient();
    virtual ~SampleClient();

    // UaSessionCallback implementation ----------------------------------------------------
    virtual void connectionStatusChanged(OpcUa_UInt32 clientConnectionId, UaClient::ServerStatus serverStatus);
    // UaSessionCallback implementation ------------------------------------------------------
    
    // set a configuration object we use to get connection parameters and NodeIds
    void setConfiguration(Configuration* pConfiguration);

    void getMethodArguments(const UaNodeId& methodId, UaArguments& inputArguments, UaArguments& outputArguments);

    void setReadValue(UaVariant value);
    UaVariant getReadValue();

    


    // OPC UA service calls
    UaStatus connect();
    UaStatus disconnect();
    UaStatus read();
    UaStatus read_specific_nodeid(std::string nodeid);
    UaStatus browseSimple();
    UaStatus browseContinuationPoint();
    UaStatus callMethods();
    UaStatus callMethodInternal(const UaNodeId& objectNodeId, const UaNodeId& methodNodeId);
    UaStatus callMethodWithValue(const UaNodeId& objectNodeId, const UaNodeId& methodNodeId, const float value);

private:
    // helper methods
    UaStatus browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn);
    void printBrowseResults(const UaReferenceDescriptions& referenceDescriptions);
    

    UaVariant m_pReadValue;

    UaSession* m_pSession;
    Configuration* m_pConfiguration;

    UaClient::ServerStatus  m_serverStatus;
};

#endif // SAMPLECLIENT_H
