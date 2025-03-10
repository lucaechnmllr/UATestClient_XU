
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "uabase.h"
#include "uaclientsdk.h"

#define COMPANY_NAME "Siemens Energy Global GmbH & Co. KG"
#define PRODUCT_NAME "UA_Test_Client"

class Configuration
{
    UA_DISABLE_COPY(Configuration);
public:
    Configuration();
    virtual ~Configuration();

    // get connection and session parameters
    UaString getServerUrl() const;
    UaString getDiscoveryUrl() const;
    UaString getApplicationName() const;
    UaString getProductUri() const;
    OpcUa_Boolean getAutomaticReconnect() const;
    OpcUa_Boolean getRetryInitialConnect() const;

    // get lists of NodeIds and values
    UaNodeIdArray getNodesToRead() const;
    UaNodeIdArray getMethodsToCall() const;
    UaNodeIdArray getObjectsToCall() const;


    // load configuration from file to fill connection parameters, NamespaceArray and NodeIds
    UaStatus loadConfiguration(const UaString& sConfigurationFile);

    // update the namespace indexes for all nodeIds and update the internal namespaceArray
    UaStatus updateNamespaceIndexes(const UaStringArray& namespaceArray);

private:
    // connection and session configuration
    UaString        m_applicationName;
    UaString        m_serverUrl;
    UaString        m_discoveryUrl;
    OpcUa_Boolean   m_bAutomaticReconnect;
    OpcUa_Boolean   m_bRetryInitialConnect;

    // NamespaceArray and NodeIds
    UaStringArray   m_namespaceArray;
    UaNodeIdArray   m_nodesToRead;
    UaNodeIdArray   m_methodsToCall;
    UaNodeIdArray   m_objectToCall;
};

#endif // CONFIGURATION_H