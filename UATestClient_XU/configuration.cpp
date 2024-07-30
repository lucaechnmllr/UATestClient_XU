#include "configuration.h"
#include "uasettings.h"

Configuration::Configuration()
{
}

Configuration::~Configuration()
{
}

UaString Configuration::getServerUrl() const
{
    return m_serverUrl;
}

UaString Configuration::getDiscoveryUrl() const
{
    return m_discoveryUrl;
}

UaString Configuration::getApplicationName() const
{
    return m_applicationName;
}

OpcUa_Boolean Configuration::getAutomaticReconnect() const
{
    return m_bAutomaticReconnect;
}

OpcUa_Boolean Configuration::getRetryInitialConnect() const
{
    return m_bRetryInitialConnect;
}

UaNodeIdArray Configuration::getNodesToRead() const
{
    return m_nodesToRead;
}

UaNodeIdArray Configuration::getMethodsToCall() const
{
    return m_methodsToCall;
}

UaNodeIdArray Configuration::getObjectsToCall() const
{
    return m_objectToCall;
}




UaStatus Configuration::loadConfiguration(const UaString& sConfigurationFile)
{
    UaStatus result;
    UaVariant value;
    UaString sTempKey;
    OpcUa_UInt32 i = 0;
    OpcUa_UInt32 size = 0;
    UaSettings* pSettings = NULL;
    pSettings = new UaSettings(sConfigurationFile.toUtf16());

    pSettings->beginGroup("UaSampleConfig");

    // Application Name
    value = pSettings->value("ApplicationName", UaString());
    m_applicationName = value.toString();

    // Server URLs
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://localhost:48010"));
    m_discoveryUrl = value.toString();
    value = pSettings->value("ServerUrl", UaString("opc.tcp://localhost:48010"));
    m_serverUrl = value.toString();

    // Reconnection settings
    value = pSettings->value("AutomaticReconnect", UaVariant((OpcUa_Boolean)OpcUa_True));
    value.toBool(m_bAutomaticReconnect);
    value = pSettings->value("RetryInitialConnect", UaVariant((OpcUa_Boolean)OpcUa_False));
    value.toBool(m_bRetryInitialConnect);

    // Read NamespaceArray
    m_namespaceArray.clear();
    pSettings->beginGroup("NSArray");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_namespaceArray.resize(size);
    for (i = 0; i < size; i++)
    {
        sTempKey = UaString("NameSpaceUri0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_namespaceArray[i]);
    }
    pSettings->endGroup();  // NSArray

    // Read NodeIds to use for reading
    m_nodesToRead.clear();
    pSettings->beginGroup("NodesToRead");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_nodesToRead.resize(size);
    for (i = 0; i < size; i++)
    {
        sTempKey = UaString("Variable0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_nodesToRead[i]);
    }
    pSettings->endGroup();  // NodesToRead


    // Read NodeIds for calling methods
    m_methodsToCall.clear();
    m_objectToCall.clear();
    pSettings->beginGroup("MethodsToCall");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_methodsToCall.resize(size);
    m_objectToCall.resize(size);
    for (i = 0; i < size; i++)
    {
        sTempKey = UaString("Method0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_methodsToCall[i]);

        sTempKey = UaString("Object0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString((value.toString())+"     ").copyTo(&m_objectToCall[i]);
    }
    pSettings->endGroup();  // MethodsToCall

    pSettings->endGroup(); // UaClientConfig

    delete pSettings;
    pSettings = NULL;

    return result;
}

UaStatus Configuration::updateNamespaceIndexes(const UaStringArray& namespaceArray)
{
    UaStatus result;
    OpcUa_UInt32 i, j;
    OpcUa_UInt32 size;

    // create mapping table
    size = m_namespaceArray.length();
    UaInt16Array mappingTable;
    mappingTable.resize(size);

    // fill mapping table
    for (i = 0; i < m_namespaceArray.length(); i++)
    {
        mappingTable[i] = (OpcUa_UInt16)i;
        // find namespace uri
        for (j = 0; j < namespaceArray.length(); j++)
        {
            UaString string1(m_namespaceArray[i]);
            UaString string2(namespaceArray[j]);
            if (string1 == string2)
            {
                mappingTable[i] = (OpcUa_UInt16)j;
                break;
            }
        }
    }

    // replace namespace index in NodeIds
    // NodesToRead
    for (i = 0; i < m_nodesToRead.length(); i++)
    {
        m_nodesToRead[i].NamespaceIndex = mappingTable[m_nodesToRead[i].NamespaceIndex];
    }


    // Methods and Objects
    for (i = 0; i < m_methodsToCall.length(); i++)
    {
        m_methodsToCall[i].NamespaceIndex = mappingTable[m_methodsToCall[i].NamespaceIndex];
    }
    for (i = 0; i < m_objectToCall.length(); i++)
    {
        m_objectToCall[i].NamespaceIndex = mappingTable[m_objectToCall[i].NamespaceIndex];
    }


    // update namespace array
    m_namespaceArray = namespaceArray;
    return result;
}