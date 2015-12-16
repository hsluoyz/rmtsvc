/*******************************************************************
*	msncx.h
*    DESCRIPTION:msnc0/msnc1协议处理类的定义声明。
*				msnp10/11现在支持msnc1.但msnp10/11仍兼容msnc0
*    AUTHOR:yyc
*
*    HISTORY:
*
*    DATE:2005-07-04
*
*******************************************************************/

#ifndef __YY_CMSNCX_H__
#define __YY_CMSNCX_H__

namespace net4cpp21
{
	class cContactor;
	class msnMessager;
	class cMsncx
	{
	protected:
		FILE *m_fp;
		bool m_bDataThread_Running;
		cContactor *m_pcontact;//本邀请隶属的聊天会话对象
		msnMessager *m_pmsnmessager;
		bool m_bSender;//是否为发送者还是接收者
		int m_inviteType;//邀请类型, INVITE_TYPE_FILE,INVITE_TYPE_PICTURE,INVITE_TYPE_ROBOT
		std::string m_filepath;//文件读取或存储路径
		std::string m_filename;//文件传输的文件名和大小
		long m_filesize;
	public:
		explicit cMsncx(msnMessager *pmsnmessager,cContactor *pcontact,int inviteType);
		virtual ~cMsncx();
		std::string &filepath() { return m_filepath;}
		std::string &filename() { return m_filename; }
		long filesize(long fs) { if(fs!=0) m_filesize=fs; return m_filesize; }
		int inviteType() const {return m_inviteType;}
		bool bSender() const { return m_bSender;}
		bool beginWrite(const char *filename,long filesize);
		bool beginWrite(){
			if(m_bSender) return false;
			std::string fullpath=m_filepath+m_filename;
			m_fp=::fopen(fullpath.c_str(),"w+b");
			return (m_fp!=NULL);
		}
		bool writeFile(const char *buf,int buflen)
		{
			if(m_fp==NULL) return false;
			::fwrite(buf,buflen,sizeof(char),m_fp);
			return true;
		}
		void endWrite(){ if(m_fp) ::fclose(m_fp); m_fp=NULL; return; }
	};
	
	class cMsnc0 : public cMsncx
	{
		socketTCP m_datasock;
		std::string m_inviteCookie;
		std::string m_authCookie;
	public:
		cMsnc0(msnMessager *pmsnmessager,cContactor *pcontact,const char *inviteCookie);
		virtual ~cMsnc0();
		bool sendmsg_ACCEPT(bool bListen);
		bool sendmsg_REJECT(const char *errCode);
		bool sendFile(const char *filename);//发送指定文件
		void setHostinfo(const char *hostip,int hostport,const char *authCookie);
		static void msnc0Thread(cMsnc0 *pmsnc0);
	};

	class cMsnc1 : public cMsncx
	{
		cCond m_cond;
		long m_BaseIdentifier;
		
		unsigned long sendMSNC1(const char *ptr_msnp2p,long p2p_length);
		unsigned long sendmsg_READY2TRANS();
		bool sendmsg_INVITE(const char *strcontext,int contextLen);
	public:
		std::string m_branch;
		std::string m_callID;
		std::string m_sessionID;
		long m_offsetIdentifier;

		cMsnc1(msnMessager *pmsnmessager,cContactor *pcontact,int inviteType=0);//MSNINVITE_TYPE_UNKNOW
		virtual ~cMsnc1();
		//获取某个联系人的头像。 saveas---图像领存为...
		bool getPicture(const char *saveas);
		bool sendPicture(const char *filename);
		bool sendFile(const char *filename);//发送指定文件
//		bool sendRobotInvite(const char *robotname); //发送机器人邀请
		//pheader 指向接收MSG的 header
		bool sendmsg_ACK(unsigned char *pheader);
		bool sendmsg_Got_BYE();
		bool sendmsg_ACCEPT();
		bool sendmsg_REJECT();
		bool sendmsg_BYE();
		bool sendmsg_ERROR();

		static void sendThread(cMsnc1 *pmsnc1);
	};

}//?namespace net4cpp21
#endif
