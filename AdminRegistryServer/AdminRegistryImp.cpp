﻿/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#include "AdminRegistryImp.h"
#include "ExecuteTask.h"
#include "servant/Application.h"

extern TC_Config * g_pconf;

void AdminRegistryImp::initialize()
{
    TLOGDEBUG("begin AdminRegistryImp init"<<endl);

    //初始化配置db连接
    //DBPROXY->init(g_pconf);

    _patchPrx = CommunicatorFactory::getInstance()->getCommunicator()->stringToProxy<PatchPrx>("tars.tarspatch.PatchObj");

    TLOGDEBUG("AdminRegistryImp init ok."<<endl);
}

int AdminRegistryImp::undeploy(const string & application, const string & serverName, const string & nodeName, const string &user, string &log, tars::TarsCurrentPtr current)
{
	TLOGDEBUG("application:" << application
		<< ", serverName:" << serverName
		<< ", nodeName:" << nodeName);

	return undeploy_inner(application, serverName, nodeName, user, log);
}
    // string info;
int AdminRegistryImp::undeploy_inner(const string & application, const string & serverName, const string & nodeName, const string &user, string &log)
{
	TLOGDEBUG("application:" << application
		<< ", serverName:" << serverName
		<< ", nodeName:" << nodeName << endl);

	stopServer_inner(application, serverName, nodeName, log);
	return DBPROXY->undeploy(application, serverName, nodeName, user, log);
}

int AdminRegistryImp::addTaskReq(const TaskReq &taskReq, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::addTaskReq taskNo:" << taskReq.taskNo <<endl);

    int ret = DBPROXY->addTaskReq(taskReq);
    if (ret != 0)
    {
        TLOGERROR("AdminRegistryImp::addTaskReq error, ret:" << ret <<endl);
        return ret;
    }

    ExecuteTask::getInstance()->addTaskReq(taskReq); 

    return 0;
}

int AdminRegistryImp::getTaskRsp(const string &taskNo, TaskRsp &taskRsp, tars::TarsCurrentPtr current)
{
    //优先从内存中获取
    bool ret = ExecuteTask::getInstance()->getTaskRsp(taskNo, taskRsp);
    if (ret)
    {
        // TLOGDEBUG("AdminRegistryImp::getTaskRsp taskNo:" << taskNo << " from running time, ret:" << ret <<endl);
        return 0;
    }

    TLOGDEBUG("AdminRegistryImp::getTaskRsp taskNo:" << taskNo << " from db."<<endl);

    return DBPROXY->getTaskRsp(taskNo, taskRsp);
}

int AdminRegistryImp::getTaskHistory(const string & application, const string & serverName, const string & command, vector<TaskRsp> &taskRsp, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::getTaskHistory application:" << application << "|serverName:" << serverName <<endl);

    return DBPROXY->getTaskHistory(application, serverName, command, taskRsp);
}

int AdminRegistryImp::setTaskItemInfo(const string & itemNo, const map<string, string> &info, tars::TarsCurrentPtr current)
{
	return setTaskItemInfo_inner(itemNo, info);
}

int AdminRegistryImp::setTaskItemInfo_inner(const string & itemNo, const map<string, string> &info)
{
	return DBPROXY->setTaskItemInfo(itemNo, info);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
///
vector<string> AdminRegistryImp::getAllApplicationNames(string & result, tars::TarsCurrentPtr current)
{
	TLOGDEBUG("into " << __FUNCTION__ << endl);
    return DBPROXY->getAllApplicationNames(result);
}

vector<string> AdminRegistryImp::getAllNodeNames(string & result, tars::TarsCurrentPtr current)
{
    map<string, string> mNodes = DBPROXY->getActiveNodeList(result);
    map<string, string>::iterator it;
    vector<string> vNodes;

    TLOGDEBUG("AdminRegistryImp::getAllNodeNames enter" <<endl);
    for(it = mNodes.begin(); it != mNodes.end(); it++)
    {
        vNodes.push_back(it->first);
    }

    return vNodes;
}

int AdminRegistryImp::getNodeVesion(const string &nodeName, string &version, string & result, tars::TarsCurrentPtr current)
{
    try
    {
        TLOGDEBUG("into " << __FUNCTION__ << endl);
        return DBPROXY->getNodeVersion(nodeName, version, result);
    }
    catch(TarsException & ex)
    {
        TLOGERROR("AdminRegistryImp::getNodeVesion '"+nodeName+"' exception:"+ex.what()<<endl);
    }
    return -1;
}

bool AdminRegistryImp::pingNode(const string & name, string & result, tars::TarsCurrentPtr current)
{
    try
    {
        TLOGDEBUG("into " << __FUNCTION__ << "|" << name << endl);
        NodePrx nodePrx = DBPROXY->getNodePrx(name);
        
        int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<ping_node_timeout>","3000"));
        nodePrx->tars_set_timeout(timeout)->tars_ping();

        result = "succ";
        TLOGDEBUG("AdminRegistryImp::pingNode name:"<<name<<"|result:"<<result<<endl);
        return true;
    }
    catch(TarsException & ex)
    {
        TLOGERROR("AdminRegistryImp::pingNode '"+name+"' exception:"+ex.what()<<endl);
        return false;
    }

    return false;
}

int AdminRegistryImp::shutdownNode(const string & name, string & result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::shutdownNode name:"<<name<<"|"<<current->getIp()<<":"<<current->getPort()<<endl);
    try
    {
        NodePrx nodePrx = DBPROXY->getNodePrx(name);
        return nodePrx->shutdown(result);
    }
    catch(TarsException & ex)
    {
        TLOGERROR( "AdminRegistryImp::shutdownNode '"<<( name + "' exception:" + ex.what())<<endl);
        return -1;
    }
}

///////////////////////////////////
vector<vector<string> > AdminRegistryImp::getAllServerIds(string & result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG(__FILE__ << "|" << __LINE__ << "|into " << __FUNCTION__ << "|" << current->getIp() << ":" << current->getPort() << endl);

    return DBPROXY->getAllServerIds(result);
}

int AdminRegistryImp::getServerState(const string & application, const string & serverName, const string & nodeName, ServerStateDesc &state, string &result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::getServerState:" << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);
        if(server.size() == 0)
        {
            result = " '" + application  + "." + serverName + "_" + nodeName + "' no config";
            TLOGERROR("AdminRegistryImp::getServerState:" << result << endl);

            return EM_TARS_LOAD_SERVICE_DESC_ERR;
        }

        state.settingStateInReg = server[0].settingState;
        state.presentStateInReg = server[0].presentState;
        state.patchVersion      = server[0].patchVersion;
        state.patchTime         = server[0].patchTime;
        state.patchUser         = server[0].patchUser;

        //判断是否为dns 非dns才需要到node调用
        if(server[0].serverType == "tars_dns")
        {
            TLOGDEBUG("AdminRegistryImp::getServerState " << ("'" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server") <<endl);
            state.presentStateInNode = server[0].presentState;
        }
        else
        {
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
            try
            {
                tars::ServerStateInfo info;
                current->setResponse(false);

                NodePrxCallbackPtr callback = new GetServerStateCallbackImp(nodePrx, application, serverName, nodeName, state, current);
                nodePrx->async_getStateInfo(callback, application, serverName);
            }
            catch(TarsException & e )
            {
                current->setResponse(true);
                string s = e.what();
                if(s.find("server function mismatch exception") != string::npos)
                {
                    ServerState stateInNode  = nodePrx->getState(application, serverName, result);
                    state.presentStateInNode = etos(stateInNode);
                    state.processId = nodePrx->getServerPid(application, serverName, result);
                }
                TLOGERROR("AdminRegistryImp::getServerState "<<"'" + application + "." + serverName + "_" + nodeName<<"|"<< e.what() <<endl);
            }
        }

        TLOGDEBUG("AdminRegistryImp::getServerState: "  << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() <<endl);

        return EM_TARS_SUCCESS;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' TarsSyncCallTimeoutException:" + ex.what();
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' TarsNodeNotRegistryException:" + ex.what();
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
    }
    catch(TarsException & ex)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' TarsException:" + ex.what();
    }
    catch(exception & tce)
    {
        result = "AdminRegistryImp::getServerState '"  + application  + "." + serverName + "_" + nodeName
                + "' exception:" + tce.what();

    }
    TLOGERROR(result << endl);
    return iRet;

}

int AdminRegistryImp::getGroupId(const string & ip, int &groupId, string &result, tars::TarsCurrentPtr current)
{
    try
    {
        TLOGDEBUG("AdminRegistryImp::getGroupId ip: "<<ip<<endl);

        return DBPROXY->getGroupId(ip);
    }
    catch(TarsException & ex)
    {
        TLOGERROR(("AdminRegistryImp::getGroupId '" + ip + "' exception:" + ex.what())<< endl);
        return -1;
    }
}

int AdminRegistryImp::startServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::startServer: "<< application << "." << serverName << "_" << nodeName
        << "|" << current->getIp() << ":" << current->getPort() <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {

        //更新数据库server的设置状态
        DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);

        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);

        //判断是否为dns 非dns才需要到node启动服务
        if(server.size() != 0 && server[0].serverType == "tars_dns")
        {
            TLOGINFO(" '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
            iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
        }
        else
        {
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
            TLOGINFO("call node into " << __FUNCTION__ << "|"
                << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() <<endl);

            current->setResponse(false);
            NodePrxCallbackPtr callback = new StartServerCallbackImp(application, serverName, nodeName, current);
            nodePrx->async_startServer(callback, application, serverName);
        }

        return iRet;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TarsRemoteNotify::getInstance()->report(string("start server:") + ex.what(), application, serverName, nodeName);
        TLOGERROR("AdminRegistryImp::startServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsSyncCallTimeoutException:" + ex.what())<<endl);
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TarsRemoteNotify::getInstance()->report(string("start server:") + ex.what(), application, serverName, nodeName);
        TLOGERROR("AdminRegistryImp::startServer '"<<(application  + "." + serverName + "_" + nodeName + "' TarsNodeNotRegistryException:" + ex.what())<<endl);
    }
    catch(TarsException & ex)
    {
        current->setResponse(true);
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' TarsException:" + ex.what();
		TarsRemoteNotify::getInstance()->report(string("start server:") + ex.what(), application, serverName, nodeName);
		 
        TLOGERROR(result << endl);
    }

	if(iRet != EM_TARS_SUCCESS)
	{
		TarsRemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
	}
    return iRet;
}
int AdminRegistryImp::startServer_inner(const string & application, const string & serverName, const string & nodeName, string &result)
{
	TLOGDEBUG("into " << __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName << endl);
	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);
		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);
		if (server.size() != 0 && server[0].serverType == "tars_dns")
		{
			TLOGINFO(" '" + application + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
			iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
        }
		else
		{
			NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			TLOGINFO("call node into " << __FUNCTION__ << "|"
				<< application << "." << serverName << "_" << nodeName << endl);
			iRet = nodePrx->startServer(application, serverName, result);
		}

		if(iRet != EM_TARS_SUCCESS)
		{
			TarsRemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
        return iRet;
	}
	catch (TarsSyncCallTimeoutException& tex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsSyncCallTimeoutException:" + tex.what();
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		TLOGERROR(result << endl);
	}
	catch (TarsNodeNotRegistryException& re)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsNodeNotRegistryException:" + re.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		TLOGERROR(result << endl);
	}
	catch (TarsException & ex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsException:" + ex.what();
		TLOGERROR(result << endl);
	}
	return iRet;
}

int AdminRegistryImp::stopServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::stopServer: "<< application << "." << serverName << "_" << nodeName
        << "|" << current->getIp() << ":" << current->getPort() <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        //更新数据库server的设置状态
        DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Inactive);

        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);

        //判断是否为dns 非dns才需要到node启动服务
        if(server.size() != 0 && server[0].serverType == "tars_dns")
        {
            TLOGINFO( "|" << " '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
            iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Inactive);
            TLOGDEBUG( __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName
                << "|" << current->getIp() << ":" << current->getPort() << "|" << iRet <<endl);
        }
        else
        {
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
            TLOGINFO("call node into " << __FUNCTION__ << "|"
                << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() <<endl);
            current->setResponse(false);
            NodePrxCallbackPtr callback = new StopServerCallbackImp(application, serverName, nodeName, current);
            nodePrx->async_stopServer(callback, application, serverName);
        }

        return iRet;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TarsRemoteNotify::getInstance()->report(string("stop server:") + ex.what(), application, serverName, nodeName);
        TLOGERROR("AdminRegistryImp::stopServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsSyncCallTimeoutException:" + ex.what())<<endl);
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
		TarsRemoteNotify::getInstance()->report(string("stop server:") + ex.what(), application, serverName, nodeName);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TarsRemoteNotify::getInstance()->report(string("stop server:") + ex.what(), application, serverName, nodeName);
        TLOGERROR("AdminRegistryImp::stopServer '"<<(application  + "." + serverName + "_" + nodeName+ "' TarsNodeNotRegistryException:" + ex.what())<<endl);
    }
    catch(TarsException & ex)
    {
        current->setResponse(true);
        result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                 + "' TarsException:" + ex.what();
        TLOGERROR(result << endl);
    }
    return iRet;
}
int AdminRegistryImp::stopServer_inner(const string & application, const string & serverName, const string & nodeName, string &result)
{
	TLOGDEBUG("into " << __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName << endl);
	int iRet = EM_TARS_UNKNOWN_ERR;
	try
	{
		if(application == "tars" && serverName == "tarsAdminRegistry")
		{
			result = "can not stop " + application + "." + serverName;
			TarsRemoteNotify::getInstance()->report(result);
			return EM_TARS_CAN_NOT_EXECUTE;
		}
		DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Inactive);
		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);
		if (server.size() != 0 && server[0].serverType == "tars_dns")
		{
			TLOGINFO( "|" << " '" + application + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
			iRet = DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Inactive);
			TLOGDEBUG( "|" << application << "." << serverName << "_" << nodeName << "|" << iRet << endl);
        }
		else
		{
			NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			TLOGINFO("call node into " << __FUNCTION__ << "|"
				<< application << "." << serverName << "_" << nodeName << endl);

			iRet = nodePrx->stopServer(application, serverName, result);		
		}

		if(iRet != EM_TARS_SUCCESS)
		{
			TarsRemoteNotify::getInstance()->report(string("stop server error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
		return iRet;
	}
	catch (TarsSyncCallTimeoutException& tex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsSyncCallTimeoutException:" + tex.what();
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		TLOGERROR(result << endl);
	}
	catch (TarsNodeNotRegistryException& re)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsNodeNotRegistryException:" + re.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		TLOGERROR(result << endl);
	}
	catch (TarsException & ex)
	{
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' TarsException:" + ex.what();
		TLOGERROR(result << endl);
	}

	return iRet;
}

int AdminRegistryImp::restartServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG(" AdminRegistryImp::restartServer: " << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() <<endl);

    bool isDnsServer = false;
    int iRet = EM_TARS_SUCCESS;
    try
    {
        vector<ServerDescriptor> server;
        server = DBPROXY->getServers(application, serverName, nodeName, true);

        //判断是否为dns 非dns才需要到node停止、启动服务
        if(server.size() != 0 && server[0].serverType == "tars_dns")
        {
            isDnsServer =  true;
        }
        else
        {
            NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			nodePrx->tars_timeout(12000);
            iRet = nodePrx->stopServer(application, serverName, result);
        }
        TLOGDEBUG("call node restartServer, stop|" << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() << endl);
	    if(iRet != EM_TARS_SUCCESS)
	    {
		    TarsRemoteNotify::getInstance()->report(string("restart server, stop error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
	    }
    }
    catch(TarsException & ex)
    {
    
        TLOGERROR(("AdminRegistryImp::restartServer '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + ex.what())<<endl);
        iRet = EM_TARS_UNKNOWN_ERR;
        TarsRemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);
    }

    if(iRet == EM_TARS_SUCCESS)
    {
        try
        {
            //从停止状态发起的restart需重设状态
            DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);

            //判断是否为dns 非dns才需要到node启动服务
            if(isDnsServer == true)
            {
                TLOGDEBUG( "|" << " '" + application  + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
                return DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
            }
            else
            {
                NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);

                TLOGDEBUG("call node restartServer(), start|" << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() << endl);

                return nodePrx->startServer(application, serverName, result);
            }
        }
        catch(TarsSyncCallTimeoutException& ex)
        {
            result = "AdminRegistryImp::restartServer '" + application  + "." + serverName + "_" + nodeName
                    + "' TarsSyncCallTimeoutException:" + ex.what();

            iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
            TarsRemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);
        }
        catch(TarsNodeNotRegistryException& ex)
        {
            result = "AdminRegistryImp::restartServer '" + application  + "." + serverName + "_" + nodeName
                    + "' TarsNodeNotRegistryException:" + ex.what();
            TarsRemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);

            iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        }
        catch (TarsException & ex)
        {
			TarsRemoteNotify::getInstance()->report(string("restart server:") + ex.what(), application, serverName, nodeName);

            result += string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                      + "' TarsException:" + ex.what();
            iRet = EM_TARS_UNKNOWN_ERR;
        }
        TLOGERROR( result << endl);
    }

    return iRet;
}
int AdminRegistryImp::restartServer_inner(const string & application, const string & serverName, const string & nodeName, string &result)
{
	TLOGDEBUG("into " << __FUNCTION__ << "|" << application << "." << serverName << "_" << nodeName << endl);
	bool isDnsServer = false;
	tarsErrCode iRet = EM_TARS_SUCCESS;
	try
	{
		vector<ServerDescriptor> server;
		server = DBPROXY->getServers(application, serverName, nodeName, true);
		if (server.size() != 0 && server[0].serverType == "tars_dns")
		{
			isDnsServer = true;
        }
		else
		{
			NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
			nodePrx->tars_timeout(12000);
			iRet = (tarsErrCode)nodePrx->stopServer(application, serverName, result);
		}
		if(iRet != EM_TARS_SUCCESS)
		{
			TarsRemoteNotify::getInstance()->report(string("restart server, stop error:" + etos((tarsErrCode)iRet)) , application, serverName, nodeName);
		}
		TLOGDEBUG("call node restartServer(), stop|" << application << "." << serverName << "_" << nodeName << "|" << etos(iRet) << endl);
	}
	catch (TarsException & ex)
    {
		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
			+ "' exception:" + ex.what();
		TLOGERROR(result << endl);
		TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);

		iRet = EM_TARS_UNKNOWN_ERR;
	}
	if (iRet == EM_TARS_SUCCESS)
	{
		try
		{
			DBPROXY->updateServerState(application, serverName, nodeName, "setting_state", tars::Active);
			if (isDnsServer == true)
			{
				TLOGDEBUG( "|" << " '" + application + "." + serverName + "_" + nodeName + "' is tars_dns server" << endl);
				return DBPROXY->updateServerState(application, serverName, nodeName, "present_state", tars::Active);
            }
			else
			{
				NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
				TLOGDEBUG("call node restartServer(), start|" << application << "." << serverName << "_" << nodeName << endl);
				return nodePrx->startServer(application, serverName, result);
			}
		}
		catch (TarsSyncCallTimeoutException& tex)
		{
			result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
				+ "' TarsSyncCallTimeoutException:" + tex.what();
			TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);

			iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		}
		catch (TarsNodeNotRegistryException& re)
		{
			result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
				+ "' TarsNodeNotRegistryException:" + re.what();
			TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);

			iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		}
		catch (TarsException & ex)
		{
			result += string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName
				+ "' TarsException:" + ex.what();
			TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);

			iRet = EM_TARS_UNKNOWN_ERR;
		}
		TLOGERROR(result << endl);
    }
    return iRet;
}

int AdminRegistryImp::notifyServer(const string & application, const string & serverName, const string & nodeName, const string &command, string &result, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::notifyServer: " << application << "." << serverName << "_" << nodeName << "|" << current->getIp() << ":" << current->getPort() <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    try
    {
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        current->setResponse(false);
        NodePrxCallbackPtr callback = new NotifyServerCallbackImp(application, serverName, nodeName, current);
        nodePrx->async_notifyServer(callback, application, serverName, command);
        return EM_TARS_SUCCESS;
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TLOGERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
                + "' TarsSyncCallTimeoutException:" + ex.what())<<endl);
        TarsRemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TLOGERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
                + "' TarsNodeNotRegistryException:" + ex.what())<<endl);
        TarsRemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }

    catch(TarsException & ex)
    {
        current->setResponse(true);
        TLOGERROR("AdminRegistryImp::notifyServer '"<<(application  + "." + serverName + "_" + nodeName
                + "' TarsException:" + ex.what())<<endl);
        TarsRemoteNotify::getInstance()->report(string("notify server:") + ex.what(), application, serverName, nodeName);
    }
    return iRet;
}

int AdminRegistryImp::batchPatch(const tars::PatchRequest & req, string & result, tars::TarsCurrentPtr current)
{
    tars::PatchRequest reqPro = req;
    reqPro.patchobj = (*g_pconf)["/tars/objname<patchServerObj>"];

    TLOGDEBUG("AdminRegistryImp::batchPatch "
                 << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|"
                 << reqPro.binname      << "|"
                 << reqPro.version      << "|"
                 << reqPro.user         << "|"
                 << reqPro.servertype   << "|"
                 << reqPro.patchobj     << "|"
                 << reqPro.md5          <<"|"
                 << reqPro.ostype
                 << endl);

    //让tarspatch准备发布包
    string sServerName  = reqPro.groupname.empty() ? reqPro.servername : reqPro.groupname;

    //获取patch包的文件信息和md5值
    string patchFile;
    string md5;
    int iRet = DBPROXY->getInfoByPatchId(reqPro.version, patchFile, md5);
    if (iRet != 0)
    {
        TLOGERROR("AdminRegistryImp::batchPatch"<< ", get patch tgz error:" << reqPro.version << endl);
        TarsRemoteNotify::getInstance()->report("get patch tgz error:" + reqPro.version, reqPro.appname, sServerName, reqPro.nodename);

        return EM_TARS_GET_PATCH_FILE_ERR;
    }

    iRet = _patchPrx->preparePatchFile(reqPro.appname, sServerName, patchFile);
    if (iRet != 0)
    {
        TLOGERROR("AdminRegistryImp::batchPatch"<< ", prepare patch file error:" << patchFile << endl);
        TarsRemoteNotify::getInstance()->report("prepare patch file error:" + patchFile, reqPro.appname, sServerName, reqPro.nodename);
        return EM_TARS_PREPARE_ERR;
    }

    reqPro.md5 = md5;

    iRet = EM_TARS_UNKNOWN_ERR;
    int defaultTime = 3000;
    NodePrx proxy;
    try
    {
        proxy = DBPROXY->getNodePrx(reqPro.nodename);
        int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<batchpatch_node_timeout>", "10000"));

        current->setResponse(false);
        NodePrxCallbackPtr callback = new PatchProCallbackImp(reqPro, proxy, defaultTime, current);
        proxy->tars_set_timeout(timeout)->async_patchPro(callback, reqPro);
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
        result = ex.what();

        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
        TLOGERROR("AdminRegistryImp::batchPatch " << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|TarsSyncCallTimeoutException:" << result << endl);
        TarsRemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);

    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        result = ex.what();
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TLOGERROR("AdminRegistryImp::batchPatch "<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|TarsNodeNotRegistryException:" << result << endl);
        TarsRemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);
    }
    catch (std::exception & ex)
    {
        current->setResponse(true);
        result = ex.what();
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
        TLOGERROR("AdminRegistryImp::batchPatch "<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename <<"|ret."<<iRet<<"|exception:" << result << endl);
        TarsRemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);
    }
    catch (...)
    {
        current->setResponse(true);
        result = "Unknown Exception";
        TLOGERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|Exception...:" << result << endl);
            TarsRemoteNotify::getInstance()->report(string("patch:") + result, reqPro.appname, sServerName, reqPro.nodename);

	}
    return iRet;
}
int AdminRegistryImp::batchPatch_inner(const tars::PatchRequest & req, string &result)
{
	tars::PatchRequest reqPro = req;
	reqPro.patchobj = (*g_pconf)["/tars/objname<patchServerObj>"];
//	reqPro.servertype = getServerType(req.appname, req.servername, req.nodename);
	TLOGDEBUG( "|"
		<< reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|"
		<< reqPro.binname << "|"
		<< reqPro.version << "|"
		<< reqPro.user << "|"
		<< reqPro.servertype << "|"
		<< reqPro.patchobj << "|"
		<< reqPro.md5 << "|"
		<< reqPro.ostype
		<< endl);
	string patchFile;
	string md5;
	int iRet = DBPROXY->getInfoByPatchId(reqPro.version, patchFile, md5);
	if (iRet != 0)
	{
		result = "get patch tgz error:" + reqPro.version;
		TLOGERROR("get patch tgz error:" << reqPro.version << endl);
		TarsRemoteNotify::getInstance()->report(result, reqPro.appname, reqPro.servername, reqPro.nodename);

		return EM_TARS_GET_PATCH_FILE_ERR;
    }

	iRet = _patchPrx->preparePatchFile(reqPro.appname, reqPro.servername, patchFile);
	if (iRet != 0)
	{
		result = "gpreparePatchFile error, check tarspatch server!";
		TLOGERROR("prepare patch file error:" << patchFile << endl);
		TarsRemoteNotify::getInstance()->report(result, reqPro.appname, reqPro.servername, reqPro.nodename);

		return EM_TARS_PREPARE_ERR;
	}
	reqPro.md5 = md5;
	iRet = EM_TARS_UNKNOWN_ERR;
	NodePrx proxy;
	try
	{
		proxy = DBPROXY->getNodePrx(reqPro.nodename);
		int timeout = TC_Common::strto<int>(g_pconf->get("/tars/nodeinfo<batchpatch_node_timeout>", "10000"));
		iRet = proxy->tars_set_timeout(timeout)->patchPro(reqPro, result);
	}
	catch (TarsSyncCallTimeoutException& tex)
	{		
		result = tex.what();
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
		TarsRemoteNotify::getInstance()->report(result, reqPro.appname, reqPro.servername, reqPro.nodename);

		TLOGERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|TarsSyncCallTimeoutException:" << result << endl);

	}
	catch (TarsNodeNotRegistryException& re)
	{
		result = re.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		TarsRemoteNotify::getInstance()->report(result, reqPro.appname, reqPro.servername, reqPro.nodename);

		TLOGERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|TarsNodeNotRegistryException:" << result << endl);
	}
	catch (std::exception & ex)
	{
		result = ex.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
		TarsRemoteNotify::getInstance()->report(result, reqPro.appname, reqPro.servername, reqPro.nodename);

		TLOGERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|exception:" << result << endl);
	}
	catch (...)
	{
		result = "Unknown Exception";
		TarsRemoteNotify::getInstance()->report(result, reqPro.appname, reqPro.servername, reqPro.nodename);

		TLOGERROR( "|" << reqPro.appname + "." + reqPro.servername + "_" + reqPro.nodename << "|ret." << iRet << "|Exception...:" << result << endl);
	}
    return iRet;
}

int AdminRegistryImp::updatePatchLog(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ, tars::TarsCurrentPtr current)
{
	return updatePatchLog_inner(application, serverName, nodeName, patchId, user, patchType, succ);
}
int AdminRegistryImp::updatePatchLog_inner(const string &application, const string & serverName, const string & nodeName, const string & patchId, const string & user, const string &patchType, bool succ)
{
	return DBPROXY->updatePatchByPatchId(application, serverName, nodeName, patchId, user, patchType, succ);
}

int AdminRegistryImp::getPatchPercent( const string& application, const string& serverName,  const string & nodeName, PatchInfo &tPatchInfo, TarsCurrentPtr current)
{
    int iRet = EM_TARS_UNKNOWN_ERR;
    string &result = tPatchInfo.sResult;
    try
    {
        TLOGDEBUG( "AdminRegistryImp::getPatchPercent: " + application  + "." + serverName + "_" + nodeName
                << "|caller: " << current->getIp()  << ":" << current->getPort() <<endl);

        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);

        current->setResponse(false);
        NodePrxCallbackPtr callback = new GetPatchPercentCallbackImp(application, serverName, nodeName, current);
        nodePrx->async_getPatchPercent(callback, application, serverName);
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        current->setResponse(true);
	    result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
        TLOGERROR(result << endl);
	    TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch(TarsNodeNotRegistryException& ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
	    result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
	    TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
        TLOGERROR(result << endl);
    }
    catch(TarsException & ex)
    {
        current->setResponse(true);
        iRet = EM_TARS_UNKNOWN_ERR;
	    result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
	    TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
	    TLOGERROR(result << endl);
    }
    return iRet;

}


int AdminRegistryImp::getPatchPercent_inner(const string &application, const string &serverName, const string & nodeName, PatchInfo &tPatchInfo)
{
	int iRet = EM_TARS_UNKNOWN_ERR;
	string &result = tPatchInfo.sResult;
	try
	{
		TLOGDEBUG( "|" + application + "." + serverName + "_" + nodeName << endl);

		NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
		
		iRet = nodePrx->getPatchPercent(application, serverName, tPatchInfo);
	}
	catch (TarsSyncCallTimeoutException& ex)
	{
//		TLOGERROR( "|" << application + "." + serverName + "_" + nodeName << "|TarsSyncCallTimeoutException:" << tex.what() << endl);
		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOGERROR(result << endl);
//		result = tex.what();
		iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
	}
	catch (TarsNodeNotRegistryException& ex)
	{
		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOGERROR(result << endl);

//		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName + "' TarsNodeNotRegistryException:" + rex.what();
		iRet = EM_TARS_NODE_NOT_REGISTRY_ERR;
//		TLOGERROR(result << endl);
	}
	catch (TarsException & ex)
	{
		result = "getPatchPercent: " + application + "." + serverName + "-" + nodeName + string(", error:") + ex.what();
		TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
		TLOGERROR(result << endl);

//		result = string(__FUNCTION__) + " '" + application + "." + serverName + "_" + nodeName + "' TarsException:" + ex.what();
		iRet = EM_TARS_UNKNOWN_ERR;
//		TLOGERROR(result << endl);
	}
	return iRet;
}

int AdminRegistryImp::loadServer(const string & application, const string & serverName, const string & nodeName, string & result, tars::TarsCurrentPtr current)
{
    try
    {
        TLOGDEBUG("AdminRegistryImp::loadServer enter"<<endl);
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        return nodePrx->loadServer(application, serverName, result);
    }
    catch(TarsSyncCallTimeoutException& ex)
    {
        TLOGERROR("AdminRegistryImp::loadServer: "<< application + "." + serverName + "_" + nodeName << "|Exception:" << ex.what() << endl);
        TarsRemoteNotify::getInstance()->report(string("loadServer:") + ex.what(), application, serverName, nodeName);
        return EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    catch(TarsNodeNotRegistryException& rex)
    {
        TLOGERROR("AdminRegistryImp::loadServer: '"<<(" '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + rex.what())<<endl);
        TarsRemoteNotify::getInstance()->report(string("loadServer:") + rex.what(), application, serverName, nodeName);
        return EM_TARS_NODE_NOT_REGISTRY_ERR;
    }
    catch(TarsException & ex)
    {
        TLOGERROR("AdminRegistryImp::loadServer: '"<<(" '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + ex.what())<<endl);
        TarsRemoteNotify::getInstance()->report(string("loadServer:") + ex.what(), application, serverName, nodeName);
        return EM_TARS_UNKNOWN_ERR;
    }

}

int AdminRegistryImp::getProfileTemplate(const std::string & profileName,std::string &profileTemplate, std::string & resultDesc, tars::TarsCurrentPtr current)
{
    profileTemplate = DBPROXY->getProfileTemplate(profileName, resultDesc);

    TLOGDEBUG("AdminRegistryImp::getProfileTemplate get " << profileName  << " return length:"<< profileTemplate.size() << endl);
    return 0;
}

int AdminRegistryImp::getServerProfileTemplate(const string & application, const string & serverName, const string & nodeName,std::string &profileTemplate, std::string & resultDesc, tars::TarsCurrentPtr current)
{
    TLOGDEBUG("AdminRegistryImp::getServerProfileTemplate get " << application<<"."<<serverName<<"_"<<nodeName<< endl);

    int iRet =  -1;
    try
    {
        if(application != "" && serverName != "" && nodeName != "")
        {
            vector<ServerDescriptor> server;
            server = DBPROXY->getServers(application, serverName, nodeName, true);
            if(server.size() > 0)
            {
                profileTemplate = server[0].profile;
                iRet = 0;
            }
        }
        TLOGDEBUG( "AdminRegistryImp::getServerProfileTemplate get " << application<<"."<<serverName<<"_"<<nodeName
            << " ret:"<<iRet<<" return length:"<< profileTemplate.size() << endl);
        return iRet;
    }
    catch(exception & ex)
    {
        resultDesc = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName
                + "' exception:" + ex.what();
        TLOGERROR(resultDesc << endl);
	    TarsRemoteNotify::getInstance()->report(resultDesc, application, serverName, nodeName);

    }
    return iRet;
}

int AdminRegistryImp::getClientIp(std::string &sClientIp,tars::TarsCurrentPtr current)
{
    sClientIp = current->getIp();
    return 0;
}
//
//int AdminRegistryImp::gridPatchServer(const vector<ServerGridDesc> &gridDescList, vector<ServerGridDesc> &gridFailDescList, std::string & resultDesc, tars::TarsCurrentPtr current)
//{
//	TLOGDEBUG(__FUNCTION__ << "|gridDescList size:" << gridDescList.size() << endl);
//
//	int iRet = 0;
//
//	try
//	{
//		gridFailDescList.clear();
//
//		for(size_t i = 0; i < gridDescList.size(); ++i)
//		{
//			if(gridDescList[i].application != "" && gridDescList[i].servername != "" && gridDescList[i].nodename != "")
//			{
//				string status("");
//
//				if(gridDescList[i].status == NORMAL)
//				{
//					status = "NORMAL";
//				}
//				else if(gridDescList[i].status == GRID)
//				{
//					status = "GRID";
//				}
//				else
//				{
//					status = "NO_FLOW";
//				}
//
//				int ret = _db.gridPatchServer(gridDescList[i].application, gridDescList[i].servername, gridDescList[i].nodename, status);
//
//				if(ret < 0)
//				{
//					gridFailDescList.push_back(gridDescList[i]);
//					iRet = -1;
//					TLOGERROR(__FUNCTION__ << "|app:" << gridDescList[i].application << "|servername:" << gridDescList[i].servername << "|node:" << gridDescList[i].nodename  << "|state:" << status << "|ret:" << ret << endl);
//					DLOG<<__FUNCTION__ << "|app:" << gridDescList[i].application << "|servername:" << gridDescList[i].servername << "|node:" << gridDescList[i].nodename  << "|state:" << status << "|ret:" << ret << endl;
//				}
//				else
//				{
//					TLOGDEBUG(__FUNCTION__ << "|app:" << gridDescList[i].application << "|servername:" << gridDescList[i].servername << "|node:" << gridDescList[i].nodename  << "|state:" << status << "|ret:" << ret << endl);
//					DLOG<<__FUNCTION__ << "|app:" << gridDescList[i].application << "|servername:" << gridDescList[i].servername << "|node:" << gridDescList[i].nodename  << "|state:" << status << "|ret:" << ret << endl;
//				}
//			}
//			else
//			{
//				TLOGERROR(__FUNCTION__ << "|app:" << gridDescList[i].application << "|servername:" << gridDescList[i].servername << "|node:" << gridDescList[i].nodename << endl);
//				DLOG<<__FUNCTION__ << "|app:" << gridDescList[i].application << "|servername:" << gridDescList[i].servername << "|node:" << gridDescList[i].nodename << endl;
//			}
//		}
//
//		return iRet;
//	}
//	catch(exception & ex)
//	{
//		iRet = EM_TARS_UNKNOWN_ERR;
//		resultDesc = string(__FUNCTION__) + "|exception:" + ex.what();
//		TLOGERROR(resultDesc << endl);
//		DLOG<<resultDesc << endl;
//	}
//
//    return iRet;
//}
//

int AdminRegistryImp::getLogFileList(const std::string & application,const std::string & serverName,const std::string & nodeName,vector<std::string> &logFileList,tars::TarsCurrentPtr current)
{
    try
    {
        TLOGDEBUG("into " << __FUNCTION__ << endl);
        string nodeIp = nodeName;

        if (nodeIp.empty())
        {
            return EM_TARS_PREPARE_ERR;
        }

        TLOGDEBUG("into " << __FUNCTION__ << "|" << application << "|" << serverName << "|" << nodeName << endl);
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        return nodePrx->getLogFileList(application, serverName, logFileList);
    }
    catch (exception & ex)
    {
    	string result = string(__FUNCTION__) + " '" + application  + "." + serverName + "_" + nodeName + string("' exception:") + ex.what();
	    TarsRemoteNotify::getInstance()->report(result, application, serverName, nodeName);
    }

    return EM_TARS_UNKNOWN_ERR;
}


int AdminRegistryImp::getLogData(const std::string & application, const std::string & serverName, const std::string & nodeName, const std::string & logFile, const std::string & cmd, std::string &fileData, tars::TarsCurrentPtr current)
{
    try
    {
        TLOGDEBUG("into " << __FUNCTION__ << endl);
        NodePrx nodePrx = DBPROXY->getNodePrx(nodeName);
        return nodePrx->getLogData(application, serverName, logFile, cmd, fileData);
    }
    catch (exception & ex)
    {
        TLOGERROR(string(__FUNCTION__) << " '" + application  << "." + serverName << "_" << nodeName << "' exception:" << ex.what() << endl);
        return EM_TARS_UNKNOWN_ERR;
    }
    return -1;
}

int AdminRegistryImp::deletePatchFile(const string &application, const string &serverName, const string & patchFile, tars::TarsCurrentPtr current)
{
	TLOGDEBUG(__FUNCTION__ << ":" << application << ", " << serverName << ", " << patchFile << endl);

	return _patchPrx->deletePatchFile(application, serverName, patchFile);
}

int AdminRegistryImp::getServers(vector<tars::FrameworkServer> &servers, tars::TarsCurrentPtr current)
{
	TLOGDEBUG(__FUNCTION__ << endl);

	int ret = DBPROXY->getFramework(servers);

	if(ret != 0)
	{
		return -1;
	}

	return 0;
}

int AdminRegistryImp::checkServer(const FrameworkServer &server, tars::TarsCurrentPtr current)
{
	TLOGDEBUG(__FUNCTION__ << ", " << server.objName << endl);

	ServantPrx prx = Application::getCommunicator()->stringToProxy<ServantPrx>(server.objName);

	try
	{
		prx->tars_ping();
	}
	catch(exception &ex)
	{
		TLOGERROR(__FUNCTION__ << ", ping: " << server.objName << ", failed:" << ex.what() << endl);
		return -1;
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////
void PatchProCallbackImp::callback_patchPro(tars::Int32 ret,
        const std::string& result)
{
    TLOGDEBUG("PatchProCallbackImp::callback_patchPro: |success ret."<<ret<<"|"
                 << _reqPro.appname + "." + _reqPro.servername + "_" + _reqPro.nodename << "|"
                 << _reqPro.binname      << "|"
                 << _reqPro.version      << "|"
                 << _reqPro.user         << "|"
                 << _reqPro.servertype   << endl);

    _nodePrx->tars_timeout(_defaultTime);

    AdminReg::async_response_batchPatch(_current, ret, result);
}

void PatchProCallbackImp::callback_patchPro_exception(tars::Int32 ret)
{
    int iRet = EM_TARS_UNKNOWN_ERR;
    _nodePrx->tars_timeout(_defaultTime);

    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }

    TarsRemoteNotify::getInstance()->report("call node to patch, timeout, ret:" + TC_Common::tostr(ret), _reqPro.appname, _reqPro.servername, _reqPro.nodename);

    AdminReg::async_response_batchPatch(_current, iRet, "");
    TLOGDEBUG("PatchProCallbackImp::callback_patchPro_exception: |exception ret."<<ret<<"|"
                 << _reqPro.appname + "." + _reqPro.servername + "_" + _reqPro.nodename << "|"
                 << _reqPro.binname      << "|"
                 << _reqPro.version      << "|"
                 << _reqPro.user         << "|"
                 << _reqPro.servertype   << endl);
}
/////////////////////////////////////////////////////////////////////////////
void StartServerCallbackImp::callback_startServer(tars::Int32 ret,
        const std::string& result)
{
    TLOGDEBUG("StartServerCallbackImp::callback_startServer: "<< "|" << _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getIp() << ":" << _current->getPort() << "|" << ret <<endl);

	if(ret != EM_TARS_SUCCESS)
	{
		TarsRemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);
	}
    AdminReg::async_response_startServer(_current, ret, result);
}

void StartServerCallbackImp::callback_startServer_exception(tars::Int32 ret)
{
    TLOGDEBUG("StartServerCallbackImp::callback_startServer_exception: "<< "|" << _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getIp() << ":" << _current->getPort() << "|" << ret <<endl);

    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }

	TarsRemoteNotify::getInstance()->report(string("start server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);

    AdminReg::async_response_startServer(_current, iRet, "");
}
/////////////////////////////////////////////////////////////////////////////
void StopServerCallbackImp::callback_stopServer(tars::Int32 ret,
        const std::string& result)
{
    TLOGDEBUG( "StopServerCallbackImp::callback_stopServer: " << _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getIp() << ":" << _current->getPort() << "|" << ret <<endl);

	if(ret != EM_TARS_SUCCESS)
	{
		TarsRemoteNotify::getInstance()->report(string("stop server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);
	}

    AdminReg::async_response_stopServer(_current, ret, result);
}

void StopServerCallbackImp::callback_stopServer_exception(tars::Int32 ret)
{
    TLOGDEBUG( "StopServerCallbackImp::callback_stopServer_exception: " << _application << "." << _serverName << "_" << _nodeName
        << "|" << _current->getIp() << ":" << _current->getPort() << "|" << ret <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }

	TarsRemoteNotify::getInstance()->report(string("stop server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);

    AdminReg::async_response_stopServer(_current, iRet, "");
}
/////////////////////////////////////////////////////////////////////////////
void NotifyServerCallbackImp::callback_notifyServer(tars::Int32 ret,  const std::string& result)
{
    TLOGDEBUG("NotifyServerCallbackImp::callback_notifyServer_exception:  "<< _current->getIp() << ":" << _current->getPort() << "|" << ret  << "|" << result <<endl);
	if(ret != EM_TARS_SUCCESS)
	{
		TarsRemoteNotify::getInstance()->report(string("notify server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);
	}

	AdminReg::async_response_notifyServer(_current, ret, result);
}

void NotifyServerCallbackImp::callback_notifyServer_exception(tars::Int32 ret)
{
    TLOGDEBUG( "NotifyServerCallbackImp::callback_notifyServer_exception " << "|" << _current->getIp() << ":" << _current->getPort() << "|" << ret <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
	TarsRemoteNotify::getInstance()->report(string("notify server error:" + etos((tarsErrCode)ret)) , _application, _serverName, _nodeName);

    AdminReg::async_response_notifyServer(_current, iRet, "");
}
/////////////////////////////////////////////////////////////////////////////
void GetServerStateCallbackImp::callback_getStateInfo(tars::Int32 ret,  const tars::ServerStateInfo& info,  const std::string& result)
{
    string resultRef = result;
    if(EM_TARS_SUCCESS == ret|| EM_TARS_UNKNOWN_ERR == ret)
    {
        _state.presentStateInNode = etos(info.serverState);
        _state.processId          = info.processId;
    }
    else
    {
        if(result.find("server function mismatch exception") != string::npos)
        {
            ServerState stateInNode   = _nodePrx->getState(_application, _serverName, resultRef);
            _state.presentStateInNode = etos(stateInNode);
            _state.processId = _nodePrx->getServerPid(_application, _serverName, resultRef);
            TLOGWARN("GetServerStateCallbackImp::callback_getStateInfo_exception '" + _application + "." + _serverName + "_" + _nodeName<<"|"<< resultRef <<endl);
        }
    }
    AdminReg::async_response_getServerState(_current, EM_TARS_SUCCESS, _state, resultRef);
    TLOGDEBUG("GetServerStateCallbackImp::callback_getStateInfo_exception " <<"|position2|"<<"'" + _application  + "." + _serverName + "_" + _nodeName<<"|"<< ret << "|" << resultRef <<endl);

}

void GetServerStateCallbackImp::callback_getStateInfo_exception(tars::Int32 ret)
{
    int iRet = EM_TARS_UNKNOWN_ERR;
    string result;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    else if(ret == tars::TARSSERVERNOFUNCERR)
    {
        iRet = EM_TARS_SUCCESS;
        ServerState stateInNode = _nodePrx->getState(_application, _serverName, result);
        _state.presentStateInNode = etos(stateInNode);
        _state.processId = _nodePrx->getServerPid(_application, _serverName, result);
        TLOGERROR("GetServerStateCallbackImp::callback_getStateInfo_exception '" + _application    + "." + _serverName + "_" + _nodeName<<"|"<< result <<endl);
    }
    AdminReg::async_response_getServerState(_current, iRet, _state, result);
    TLOGERROR("GetServerStateCallbackImp::callback_getStateInfo_exception " <<"|position3|"<<"'" + _application  + "." + _serverName + "_" + _nodeName<<"|"<< ret << "|" << iRet <<endl);
}
/////////////////////////////////////////////////////////////////////////////
void GetPatchPercentCallbackImp::callback_getPatchPercent(tars::Int32 ret,  const tars::PatchInfo& tPatchInfo)
{
    AdminReg::async_response_getPatchPercent(_current, ret, tPatchInfo);
    TLOGDEBUG("GetPatchPercentCallbackImp::callback_getPatchPercent " <<"|"<< _application + "." + _serverName + "_" + _nodeName<<"|"<< ret <<endl);
}

void GetPatchPercentCallbackImp::callback_getPatchPercent_exception(tars::Int32 ret)
{
    TLOGERROR("GetPatchPercentCallbackImp::callback_getPatchPercent_exception " <<"|"<< _application + "." + _serverName + "_" + _nodeName<<"|"<< ret <<endl);
    int iRet = EM_TARS_UNKNOWN_ERR;
    if(ret == tars::TARSSERVERQUEUETIMEOUT || ret == tars::TARSASYNCCALLTIMEOUT)
    {
        iRet = EM_TARS_CALL_NODE_TIMEOUT_ERR;
    }
    tars::PatchInfo tPatchInfo;
    AdminReg::async_response_getPatchPercent(_current, iRet, tPatchInfo);
}

