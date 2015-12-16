
/*******************************************************************
   *	Handle.h
   *    DESCRIPTION:自动变量
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_THandle_H__
#define __YY_THandle_H__

namespace net4cpp21
{

	template<class X> class THandle 
	{
	private:
		X* rep;
		int* pcount;
	public:
		X* operator->() { return rep; }
		const X* operator->() const { return rep; }
		X& operator*() { return *rep; }
		const X& operator*() const { return *rep; }

		THandle(X* pp = 0) : rep(pp), pcount(new int(1)) {}
		THandle(const THandle& r) : rep(r.rep), pcount(r.pcount) { (*pcount)++; }

		THandle& operator=(const THandle& r)
		{
			if (rep == r.rep) return *this;
			if (--(*pcount) == 0) {
				delete rep;
				delete pcount;
			}
			rep = r.rep;
			pcount = r.pcount;
			(*pcount)++;
			return *this;
		}
	
		~THandle() { if (--(*pcount) == 0) { delete rep; delete pcount; }}

		X* get_rep() { return rep; }
		const X* get_rep() const { return rep; }

		void bind(X* pp)
		{
			if (pp != rep) {
				if (--(*pcount) == 0) {
					delete rep;
					*pcount = 1;			// recycle pcount
				}
				else
					pcount = new int(1);	// new pcount,有其他地方引用原rep
				rep = pp;
			}
		}//?void bind(X* pp)

		X * release(){
			X *p=NULL;
			if( *pcount==1 ){ //如果没有地方引用原rep则允许释放，否则不允许
			//因为如果其他地方还引用了原rep如果此处释放有可能会产生野指针问题，即某个地方释放了，其他地方还在引用
				p=rep; rep=NULL;
			}
			return p;
		}
	};


}//?namespace net4cpp21
#endif //__THandle_H__
