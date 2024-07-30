#include "sampleclient.h"
#include "uasession.h"
#include "configuration.h"
#include "uaargument.h"
#include <iostream>
#include <ctime>
#include <string>
#include <iostream>


SampleClient::SampleClient()
{
    m_pSession = new UaSession();

    m_pConfiguration = new Configuration();

    std::srand((unsigned int)std::time(NULL));
}

SampleClient::~SampleClient()
{
    if (m_pSession)
    {
        // disconnect if we're still connected
        if (m_pSession->isConnected() != OpcUa_False)
        {
            ServiceSettings serviceSettings;
            m_pSession->disconnect(serviceSettings, OpcUa_True);
        }
        
        delete m_pSession;
        m_pSession = NULL;
    }
    if (m_pConfiguration)
    {
        delete m_pConfiguration;
        m_pConfiguration = NULL;
    }
}

void SampleClient::connectionStatusChanged(
    OpcUa_UInt32             clientConnectionId,
    UaClient::ServerStatus   serverStatus)
{
    OpcUa_ReferenceParameter(clientConnectionId);

    fprintf(stderr,"-------------------------------------------------------------\n");
    switch (serverStatus)
    {
    case UaClient::Disconnected:
        fprintf(stderr,"Connection status changed to Disconnected\n");
        break;
    case UaClient::Connected:
        fprintf(stderr,"Connection status changed to Connected\n");
        if (m_serverStatus != UaClient::NewSessionCreated)
        {
            m_pConfiguration->updateNamespaceIndexes(m_pSession->getNamespaceTable());
        }
        break;
    case UaClient::ConnectionWarningWatchdogTimeout:
        fprintf(stderr,"Connection status changed to ConnectionWarningWatchdogTimeout\n");
        break;
    case UaClient::ConnectionErrorApiReconnect:
        fprintf(stderr,"Connection status changed to ConnectionErrorApiReconnect\n");
        break;
    case UaClient::ServerShutdown:
        fprintf(stderr,"Connection status changed to ServerShutdown\n");
        break;
    case UaClient::NewSessionCreated:
        fprintf(stderr,"Connection status changed to NewSessionCreated\n");
        m_pConfiguration->updateNamespaceIndexes(m_pSession->getNamespaceTable());
        break;
    }
    fprintf(stderr,"-------------------------------------------------------------\n");

    m_serverStatus = serverStatus;
}


UaStatus SampleClient::connect()
{
    UaStatus result;

    // Provide information about the client
    SessionConnectInfo sessionConnectInfo;
    UaString sNodeName("unknown_host");
    char szHostName[256];
    if (0 == UA_GetHostname(szHostName, 256))
    {
        sNodeName = szHostName;
    }

    // Fill session connect info with data from configuration
    sessionConnectInfo.sApplicationName = m_pConfiguration->getApplicationName();
    // Use the host name to generate a unique application URI
    sessionConnectInfo.sApplicationUri = UaString("urn:%1:%2:%3").arg(sNodeName).arg(COMPANY_NAME).arg(PRODUCT_NAME);
    sessionConnectInfo.sProductUri = UaString("urn:%1:%2").arg(COMPANY_NAME).arg(PRODUCT_NAME);
    sessionConnectInfo.sSessionName = sessionConnectInfo.sApplicationUri;
    sessionConnectInfo.bAutomaticReconnect = m_pConfiguration->getAutomaticReconnect();
    sessionConnectInfo.bRetryInitialConnect = m_pConfiguration->getRetryInitialConnect();

    // Security settings are not initialized - we connect without security for now
    SessionSecurityInfo sessionSecurityInfo;

    fprintf(stderr,"\nConnecting to %s\n", m_pConfiguration->getServerUrl().toUtf8());
    result = m_pSession->connect(
        m_pConfiguration->getServerUrl(),
        sessionConnectInfo,
        sessionSecurityInfo,
        this);

    if (result.isGood())
    {
        fprintf(stderr,"Connect succeeded\n");
    }
    else
    {
        fprintf(stderr,"Connect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}


UaStatus SampleClient::disconnect()
{
    UaStatus result;

    // Default settings like timeout
    ServiceSettings serviceSettings;

    fprintf(stderr,"\nDisconnecting ...\n");
    result = m_pSession->disconnect(
        serviceSettings,
        OpcUa_True);

    if (result.isGood())
    {
        fprintf(stderr,"Disconnect succeeded\n");
    }
    else
    {
        fprintf(stderr,"Disconnect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::read_specific_nodeid(std::string nodeid)
{
    UaStatus          result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodeToRead;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;


    nodeToRead.create(1);

    OpcUa_UInt16 namespaceIndex = 2;
    UaNodeId nodeId(nodeid.c_str(), namespaceIndex);

    nodeId.copyTo(&nodeToRead[0].NodeId);
    nodeToRead[0].AttributeId = OpcUa_Attributes_Value;

    result = m_pSession->read(
        serviceSettings,
        0,
        OpcUa_TimestampsToReturn_Both,
        nodeToRead,
        values,
        diagnosticInfos);

    //std::cout << "Reading node id: " << nodeid << std::endl;
    if (result.isGood())
    {
        // Read service succeded - check status of read value
        if (OpcUa_IsGood(values[0].StatusCode))
        {
            //printf("Value: %s\n", UaVariant(values[0].Value).toString().toUtf8());
            setReadValue(values[0].Value);
        }
        else
        {
            printf("Read failed for item[0] with status %s\n", UaStatus(values[0].StatusCode).toString().toUtf8());
        }
    }
    else
    {
        // Service call failed
        printf("Read failed with status %s\n", result.toString().toUtf8());
    }

    return result;

}

UaStatus SampleClient::read()
{
    UaStatus          result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodesToRead;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;

    // read all nodes from the configuration
    UaNodeIdArray nodes = m_pConfiguration->getNodesToRead();
    nodesToRead.create(nodes.length());

    for (OpcUa_UInt32 i = 0; i < nodes.length(); i++)
    {
        nodesToRead[i].AttributeId = OpcUa_Attributes_Value;
        OpcUa_NodeId_CopyTo(&nodes[i], &nodesToRead[i].NodeId);
    }

    //fprintf(stderr,"\nReading ...\n");
    result = m_pSession->read(
        serviceSettings,
        0,
        OpcUa_TimestampsToReturn_Both,
        nodesToRead,
        values,
        diagnosticInfos);

    if (result.isGood())
    {
        // Read service succeded - check individual status codes
        for (OpcUa_UInt32 i = 0; i < nodes.length(); i++)
        {
            if (OpcUa_IsGood(values[i].StatusCode))
            {
                //fprintf(stderr,"Value[%d]: %s\n", i, UaVariant(values[i].Value).toString().toUtf8());
                setReadValue(values[i].Value);
            }
            else
            {
                fprintf(stderr,"Read failed for item[%d] with status %s\n", i, UaStatus(values[i].StatusCode).toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        fprintf(stderr,"Read failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}


void SampleClient::setReadValue(UaVariant value)
{
    m_pReadValue = value;
}

UaVariant SampleClient::getReadValue()
{
    UaVariant tempValue(m_pReadValue);
    tempValue.changeType((OpcUa_BuiltInType)OpcUaType_Float, OpcUa_False);
    return tempValue;
}



UaStatus SampleClient::browseSimple()
{
    UaStatus result;
    UaNodeId nodeToBrowse;

    // browse from root folder with no limitation of references to return
    nodeToBrowse = UaNodeId(OpcUaId_RootFolder);
    result = browseInternal(nodeToBrowse, 0);

    return result;
}

UaStatus SampleClient::browseContinuationPoint()
{
    UaStatus result;
    UaNodeId nodeToBrowse;

    // browse from Massfolder with max references to return set to 5
    nodeToBrowse = UaNodeId("XU", 2);
    result = browseInternal(nodeToBrowse, 5);

    return result;
}

UaStatus SampleClient::browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
{
    UaStatus result;

    ServiceSettings serviceSettings;
    BrowseContext browseContext;
    UaByteString continuationPoint;
    UaReferenceDescriptions referenceDescriptions;

    // configure browseContext
    browseContext.browseDirection = OpcUa_BrowseDirection_Forward;
    browseContext.referenceTypeId = OpcUaId_HierarchicalReferences;
    browseContext.includeSubtype = OpcUa_True;
    browseContext.maxReferencesToReturn = maxReferencesToReturn;

    fprintf(stderr,"\nBrowsing from Node %s...\n", nodeToBrowse.toXmlString().toUtf8());
    result = m_pSession->browse(
        serviceSettings,
        nodeToBrowse,
        browseContext,
        continuationPoint,
        referenceDescriptions);

    if (result.isGood())
    {
        // print results
        printBrowseResults(referenceDescriptions);

        // continue browsing
        while (continuationPoint.length() > 0)
        {
            fprintf(stderr,"\nContinuationPoint is set. BrowseNext...\n");
            // browse next
            result = m_pSession->browseNext(
                serviceSettings,
                OpcUa_False,
                continuationPoint,
                referenceDescriptions);

            if (result.isGood())
            {
                // print results
                printBrowseResults(referenceDescriptions);
            }
            else
            {
                // Service call failed
                fprintf(stderr,"BrowseNext failed with status %s\n", result.toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        fprintf(stderr,"Browse failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

void SampleClient::printBrowseResults(const UaReferenceDescriptions& referenceDescriptions)
{
    OpcUa_UInt32 i;
    for (i = 0; i < referenceDescriptions.length(); i++)
    {
        fprintf(stderr,"node: ");
        UaNodeId referenceTypeId(referenceDescriptions[i].ReferenceTypeId);
        fprintf(stderr,"[Ref=%s] ", referenceTypeId.toString().toUtf8());
        UaQualifiedName browseName(referenceDescriptions[i].BrowseName);
        fprintf(stderr,"%s ( ", browseName.toString().toUtf8());
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Object) fprintf(stderr,"Object ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Variable) fprintf(stderr,"Variable ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Method) fprintf(stderr,"Method ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ObjectType) fprintf(stderr,"ObjectType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_VariableType) fprintf(stderr,"VariableType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ReferenceType) fprintf(stderr,"ReferenceType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_DataType) fprintf(stderr,"DataType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_View) fprintf(stderr,"View ");
        UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
        fprintf(stderr,"[NodeId=%s] ", nodeId.toFullString().toUtf8());
        fprintf(stderr,")\n");
    }
}

void SampleClient::setConfiguration(Configuration* pConfiguration)
{
    if (m_pConfiguration)
    {
        delete m_pConfiguration;
    }
    m_pConfiguration = pConfiguration;
}

UaStatus SampleClient::callMethods()
{
    UaStatus result;

    // call all methods from the configuration
    UaNodeIdArray objectNodeIds = m_pConfiguration->getObjectsToCall();
    UaNodeIdArray methodNodeIds = m_pConfiguration->getMethodsToCall();

    for (OpcUa_UInt32 i = 0; i < methodNodeIds.length(); i++)
    {
        UaStatus stat = callMethodInternal(objectNodeIds[i], methodNodeIds[i]);
        if (stat.isNotGood())
        {
            result = stat;
        }
    }

    return result;
}

UaStatus SampleClient::callMethodInternal(const UaNodeId& objectNodeId, const UaNodeId& methodNodeId)
{
    UaStatus        result;
    ServiceSettings serviceSettings;
    CallIn          callRequest;
    CallOut         callResult;
    static int      value;


    // NodeIds for Object and Method
    // we set no call parameters here
    callRequest.methodId = methodNodeId;
    callRequest.objectId = objectNodeId;


    UaArguments inputArguments;
    UaArguments outputArguments;
    getMethodArguments(callRequest.methodId, inputArguments, outputArguments);

    if (inputArguments.length() > 0)
    {
        // Create input arguments value array
        callRequest.inputArguments.create(inputArguments.length());


        /*if (value == 0)
            value = 1;
        else
            value = 0;*/
        
        // Set input arguments 
        // Create random number for value
        OpcUa_Float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        UaVariant tempValue(r + (rand()%10));
        //UaVariant tempValue(value);
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[0].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[0]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[1].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[1]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[2].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[2]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[3].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[3]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[4].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[4]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[5].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[5]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[6].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[6]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[7].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[7]);

    }


    //fprintf(stderr,"\nCalling method writeSignal on object %s\n", objectNodeId.toString().toUtf8());
    result = m_pSession->call(
        serviceSettings,
        callRequest,
        callResult);

    if (result.isGood())
    {
        // Call service succeded - check result
        if (callResult.callResult.isGood())
        {
            //fprintf(stderr,"Call succeeded\n");
        }
        else
        {
            //fprintf(stderr,"Call failed with status %s\n", callResult.callResult.toString().toUtf8());
        }

    }
    else
    {
        // Service call failed
        //fprintf(stderr,"Call failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::callMethodWithValue(const UaNodeId& objectNodeId, const UaNodeId& methodNodeId, const float value)
{
    UaStatus        result;
    ServiceSettings serviceSettings;
    CallIn          callRequest;
    CallOut         callResult;


    // NodeIds for Object and Method
    // we set no call parameters here
    callRequest.methodId = methodNodeId;
    callRequest.objectId = objectNodeId;


    UaArguments inputArguments;
    UaArguments outputArguments;
    getMethodArguments(callRequest.methodId, inputArguments, outputArguments);

    if (inputArguments.length() > 0)
    {
        // Create input arguments value array
        callRequest.inputArguments.create(inputArguments.length());

        UaVariant tempValue(value);
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[0].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[0]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[1].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[1]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[2].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[2]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[3].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[3]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[4].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[4]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[5].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[5]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[6].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[6]);
        tempValue = OpcUa_False;
        tempValue.changeType((OpcUa_BuiltInType)inputArguments[7].DataType.Identifier.Numeric, OpcUa_False);
        tempValue.copyTo(&callRequest.inputArguments[7]);

    }


    //fprintf(stderr,"\nCalling method writeSignal on object %s\n", objectNodeId.toString().toUtf8());
    result = m_pSession->call(
        serviceSettings,
        callRequest,
        callResult);

    if (result.isGood())
    {
        // Call service succeded - check result
        if (callResult.callResult.isGood())
        {
            //fprintf(stderr, "Call succeeded\n");
        }
        else
        {
            //fprintf(stderr,"Call failed with status %s\n", callResult.callResult.toString().toUtf8());
        }

    }
    else
    {
        // Service call failed
        //fprintf(stderr,"Call failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

void SampleClient::getMethodArguments(const UaNodeId& methodId, UaArguments& inputArguments, UaArguments& outputArguments)
{
    UaStatus                status;
    UaDiagnosticInfos       diagnosticInfos;
    ServiceSettings         serviceSettings;
    UaBrowsePaths           browsePaths;
    UaBrowsePathResults     browsePathResults;
    UaRelativePathElements  pathElements;
    UaReadValueIds          nodesToRead;
    UaDataValues            values;

    browsePaths.create(2);
    // InputArguments
    methodId.copyTo(&browsePaths[0].StartingNode);
    pathElements.create(1);
    pathElements[0].ReferenceTypeId.Identifier.Numeric = OpcUaId_HasProperty;
    OpcUa_String_AttachReadOnly(&pathElements[0].TargetName.Name, "InputArguments");
    pathElements[0].TargetName.NamespaceIndex = 0;
    browsePaths[0].RelativePath.NoOfElements = pathElements.length();
    browsePaths[0].RelativePath.Elements = pathElements.detach();
    // OutputArguments
    methodId.copyTo(&browsePaths[0].StartingNode);
    pathElements.create(1);
    pathElements[0].ReferenceTypeId.Identifier.Numeric = OpcUaId_HasProperty;
    OpcUa_String_AttachReadOnly(&pathElements[0].TargetName.Name, "OutputArguments");
    pathElements[0].TargetName.NamespaceIndex = 0;
    browsePaths[1].RelativePath.NoOfElements = pathElements.length();
    browsePaths[1].RelativePath.Elements = pathElements.detach();

    // Try to get the NodeIds of Method InputArguments and OutputArguments properties
    status = m_pSession->translateBrowsePathsToNodeIds(
        serviceSettings, // Use default settings
        browsePaths,
        browsePathResults,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad())
    {
        return;
    }

    nodesToRead.create(2);
    nodesToRead[0].AttributeId = OpcUa_Attributes_Value;
    if (browsePathResults[0].NoOfTargets > 0)
    {
        // We use the first result if provided - the first is from the type definition
        UaNodeId::cloneTo(browsePathResults[0].Targets[0].TargetId.NodeId, nodesToRead[0].NodeId);
    }
    nodesToRead[1].AttributeId = OpcUa_Attributes_Value;
    if (browsePathResults[1].NoOfTargets > 0)
    {
        UaNodeId::cloneTo(browsePathResults[1].Targets[0].TargetId.NodeId, nodesToRead[1].NodeId);
    }

    status = m_pSession->read(
        serviceSettings,
        0,
        OpcUa_TimestampsToReturn_Neither,
        nodesToRead,
        values,
        diagnosticInfos);
    if (status.isBad())
    {
        return;
    }

    // Check if we have InputArguments
    if (OpcUa_IsGood(values[0].StatusCode))
    {
        UaVariant vValue(values[0].Value);
        if (vValue.arrayType() == OpcUa_VariantArrayType_Array && vValue.dataType() == OpcUaId_Argument)
        {
            inputArguments.setArguments(vValue, OpcUa_True);
        }
    }

    // Check if we have OutputArguments
    if (OpcUa_IsGood(values[1].StatusCode))
    {
        UaVariant vValue(values[1].Value);
        if (vValue.arrayType() == OpcUa_VariantArrayType_Array && vValue.dataType() == OpcUaId_Argument)
        {
            outputArguments.setArguments(vValue, OpcUa_True);
        }
    }
}