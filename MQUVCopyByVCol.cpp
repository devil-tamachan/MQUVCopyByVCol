//---------------------------------------------------------------------------
//
//   MQUVCopyByVCol
//
//          Copyright(C) 1999-2013, tetraface Inc.
//
//     Sample for Object plug-in for deleting faces with reserving lines
//    in the faces.
//     Created DLL must be installed in "Plugins\Object" directory.
//
//    　オブジェクト中の辺のみを残して面を削除するオブジェクトプラグイン
//    のサンプル。
//    　作成したDLLは"Plugins\Object"フォルダに入れる必要がある。
//
//
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>
#include "MQPlugin.h"
#include "MQBasePlugin.h"
#include "MQWidget.h"

HINSTANCE hInstance;



//---------------------------------------------------------------------------
//  DllMain
//---------------------------------------------------------------------------
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	// リソース表示に必要なので、インスタンスを保存しておく
	// Store the instance because it is necessary for displaying resources.
	hInstance = (HINSTANCE)hModule;

	// プラグインとしては特に必要な処理はないので、何もせずにTRUEを返す
	// There is nothing to do for a plugin. So just return TRUE.
    return TRUE;
}


//---------------------------------------------------------------------------
//  class MQUVCopyByVColPlugin
//---------------------------------------------------------------------------
class MQUVCopyByVColPlugin : public MQObjectPlugin
{
public:
	MQUVCopyByVColPlugin();
	~MQUVCopyByVColPlugin();

	void GetPlugInID(DWORD *Product, DWORD *ID) override;
	const char *GetPlugInName(void) override;
	const char *EnumString(int index) override;
	BOOL Execute(int index, MQDocument doc) override;

private:
	BOOL MQUVCopyByVCol(MQDocument doc);
	BOOL DeleteLines(MQDocument doc);
};

MQBasePlugin *GetPluginClass()
{
	static MQUVCopyByVColPlugin plugin;
	return &plugin;
}


MQUVCopyByVColPlugin::MQUVCopyByVColPlugin()
{
}

MQUVCopyByVColPlugin::~MQUVCopyByVColPlugin()
{
}

//---------------------------------------------------------------------------
//  MQGetPlugInID
//    プラグインIDを返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
void MQUVCopyByVColPlugin::GetPlugInID(DWORD *Product, DWORD *ID)
{
	// プロダクト名(制作者名)とIDを、全部で64bitの値として返す
	// 値は他と重複しないようなランダムなもので良い
	*Product = 0x8AFEA457;
	*ID      = 0x9F57C622;
}

//---------------------------------------------------------------------------
//  MQGetPlugInName
//    プラグイン名を返す。
//    この関数は[プラグインについて]表示時に呼び出される。
//---------------------------------------------------------------------------
const char *MQUVCopyByVColPlugin::GetPlugInName(void)
{
	// プラグイン名
	return "MQUVCopyByVCol           Copyright(C) 2016, tamachan";
}

//---------------------------------------------------------------------------
//  MQEnumString
//    ポップアップメニューに表示される文字列を返す。
//    この関数は起動時に呼び出される。
//---------------------------------------------------------------------------
const char *MQUVCopyByVColPlugin::EnumString(int index)
{
	static char buffer[256];

	switch(index){
	case 0:
		// Note: following $res$ means the string uses system resource string.
		return "頂点色が同じUVをコピー";
	}
	return NULL;
}

//---------------------------------------------------------------------------
//  MQModifyObject
//    メニューから選択されたときに呼び出される。
//---------------------------------------------------------------------------
BOOL MQUVCopyByVColPlugin::Execute(int index, MQDocument doc)
{
	switch(index){
	case 0: return MQUVCopyByVCol(doc);
	}
	return FALSE;
}


int compare_VCols(const void *a, const void *b)
{
  const BYTE *p1 = (const BYTE*)a;
  const BYTE *p2 = (const BYTE*)b;
  for(int i=0;i<13/*rgb*4+fTri*/;i++)
  {
    if(*p1<*p2)return -1;
    else if(*p1>*p2)return 1;
    p1++;
    p2++;
  }
  return 0;
}

static const DWORD g_VColUVs_OneSetSize = 3/*RGB*/*4/*角形*/+1/*fTri*/ + sizeof(MQCoordinate)*4/*角形*/;


int SearchVCols(BYTE *pVColUVs, DWORD numSet, BYTE *vcols)
{
	int mid;
  int left = 0;
  int right = numSet-1;

	while (left < right) {
		mid = (left + right) / 2;
		if (compare_VCols((const void*)(pVColUVs + g_VColUVs_OneSetSize * mid), vcols) < 0) left = mid + 1;  else right = mid;
	}
	if (compare_VCols((const void*)(pVColUVs + g_VColUVs_OneSetSize * left), vcols) == 0) return left;
	return -1;
}

int SearchMinVColIdx(BYTE *vcols, int numV)
{
  int minIdx=0;
  for(int i=1;i<numV;i++)
  {
    if(compare_VCols(vcols+i*3, vcols+minIdx*3)<0)minIdx=i;
  }
  return minIdx;
}

void rotateL(BYTE *arr, int unitSize, int numUnit, int shift)
{
  if(shift>=numUnit)shift%=numUnit;
  if(shift==0)return;
  BYTE *t = NULL;
  if(numUnit/2 < shift)
  {//right rotate
    shift = numUnit - shift;
    int tSize = shift*unitSize;
    BYTE *t = (BYTE *)malloc(tSize);
    memcpy(t, arr+(numUnit-shift)*unitSize, tSize);
    memmove(arr+shift*unitSize, arr, (numUnit-shift)*unitSize);
    memcpy(arr, t, tSize);
  } else {//left rotate
    int tSize = shift*unitSize;
    BYTE *t = (BYTE *)malloc(tSize);
    memcpy(t, arr, tSize);
    memmove(arr, arr+shift*unitSize, (numUnit-shift)*unitSize);
    memcpy(arr+(numUnit-shift)*unitSize, t, tSize);
  }
  free(t);
}
void rotateR(BYTE *arr, int unitSize, int numUnit, int shift)
{
  if(shift>=numUnit)shift%=numUnit;
  if(shift==0)return;
  int lshift = numUnit-shift;
  rotateL(arr, unitSize, numUnit, lshift);
}
int SortVCols(BYTE *vcols, int numV)
{
  int minIdx = SearchMinVColIdx(vcols, numV);
  if(minIdx==0)return minIdx;
  rotateL(vcols, 3, numV, minIdx);
  return minIdx;
}
void SortUVs(BYTE *pVColUVs, int numV, int minIdx)
{
  rotateL(pVColUVs + 13, sizeof(MQCoordinate), numV, minIdx);
}
int SortVColUV(BYTE *pVColUVs)
{
  int numV = pVColUVs[12]==1?3:4;
  int minIdx = SortVCols(pVColUVs, numV);
  if(minIdx>0)SortUVs(pVColUVs, numV, minIdx);
  return minIdx;
}

void DbgVColUVs(BYTE *p)
{
  CString s;
  s.Format("%02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X %02X %f,%f %f,%f %f,%f %f,%f", p[0],p[1],p[2], p[3],p[4],p[5], p[6],p[7],p[8], p[9],p[10],p[11],    p[12],   *(float*)(p+13),*(float*)(p+17), *(float*)(p+21),*(float*)(p+25), *(float*)(p+29),*(float*)(p+33), *(float*)(p+37),*(float*)(p+41));
  OutputDebugString(s);
}
void DbgVCols(BYTE *p)
{
  CString s;
  s.Format("%02X%02X%02X %02X%02X%02X %02X%02X%02X %02X%02X%02X %02X", p[0],p[1],p[2], p[3],p[4],p[5], p[6],p[7],p[8], p[9],p[10],p[11],    p[12]);
  OutputDebugString(s);
}

DWORD ReadVColUVs(MQObject oBrush, BYTE *pVColUVs)
{
  DWORD numSet = 0;
  BYTE *pCur = pVColUVs;
  for(int fi=0;fi<oBrush->GetFaceCount();fi++)
  {
    int numV = oBrush->GetFacePointCount(fi);
    if(numV>=5)
    {
      MessageBox(NULL, "brushオブジェクトに５角以上のポリゴンが含まれています", "エラー", MB_ICONSTOP);
      return 0;
    }
    if(numV<=2)continue;
    for(int j=0;j<numV;j++)
    {
      DWORD c = oBrush->GetFaceVertexColor(fi, j);
      memcpy(pCur, &c, 3);
      pCur+=3;
    }
    if(numV==3)
    {
      memset(pCur, 0, 3);
      pCur+=3;
      *pCur=1;
    } else *pCur=0;
    pCur++;

    oBrush->GetFaceCoordinateArray(fi, (MQCoordinate*)pCur);
    pCur+=sizeof(MQCoordinate)*4;

    numSet++;
  }
  pCur = pVColUVs;
  for(int i=0;i<numSet;i++)
  {
    //OutputDebugString(pCur[12]?"3":"4");
    //DbgVColUVs(pCur);
    SortVColUV(pCur);
    //DbgVColUVs(pCur);
    pCur+=g_VColUVs_OneSetSize;
  }
    //OutputDebugString("++++++++++++++++++++++Sorted src");
  /*pCur = pVColUVs;
  for(int i=0;i<numSet;i++)
  {
    DbgVColUVs(pCur);
    pCur+=g_VColUVs_OneSetSize;
  }*/
    //OutputDebugString("----------------------Sorted src");

  /*FILE *fp = fopen("C:\\tmp\\dbg2.bin", "wb");
  if(fp!=NULL)
  {
    fwrite(pVColUVs, g_VColUVs_OneSetSize, numSet, fp);
    fclose(fp);
  }*/
  qsort(pVColUVs, numSet, g_VColUVs_OneSetSize, compare_VCols);
  //  OutputDebugString("++++++++++++++++++++++Sorted src2");
  /*pCur = pVColUVs;
  for(int i=0;i<numSet;i++)
  {
    DbgVColUVs(pCur);
    pCur+=g_VColUVs_OneSetSize;
  }*/
  //  OutputDebugString("----------------------Sorted src2");
  return numSet;
}

void SetUVs(MQObject o, int fi, BYTE *pVColUVs, int idx, int rshift)
{
  MQCoordinate uvs[4];
  /*{
    CString s;
    s.Format("idx==%d\n", idx);
    OutputDebugString(s);
  }*/
  int numV = pVColUVs[12]==1 ? 3:4;
  if(numV!=o->GetFacePointCount(fi))return;
  void *pStart = (pVColUVs + idx*g_VColUVs_OneSetSize + 13);
  memcpy(uvs, pStart, sizeof(MQCoordinate)*numV);
  /*for(int i=0;i<numV;i++)
  {
    CString s;
    s.Format("%f,%f\n", uvs[i].u, uvs[i].v);
    OutputDebugString(s);
  }*/
    //OutputDebugString("------------------");
  rotateR((BYTE*)uvs, sizeof(MQCoordinate), numV, rshift);
  /*for(int i=0;i<numV;i++)
  {
    CString s;
    s.Format("%f,%f\n", uvs[i].u, uvs[i].v);
    OutputDebugString(s);
  }*/
    //OutputDebugString("------------------");
  o->SetFaceCoordinateArray(fi, uvs);
}

BOOL MQUVCopyByVColPlugin::MQUVCopyByVCol(MQDocument doc)
{
  BYTE *pVColUVs = NULL;
  CAtlArray<MQObject> targetObjArr;
  MQObject oBrush = NULL;
  int numObj = doc->GetObjectCount();
  for(int i=0;i<numObj;i++)
  {
    MQObject o = doc->GetObject(i);
    if(o!=NULL)
    {
      CString objName;
      o->GetName(objName.GetBufferSetLength(151), 150);
      objName.ReleaseBuffer();
      if(objName==_T("brush"))oBrush = o;
      else {
        targetObjArr.Add(o);
      }
    }
  }
  
  if(oBrush == NULL || oBrush->GetFaceCount()==0 ||targetObjArr.GetCount()==0)return FALSE;
  
  pVColUVs = (BYTE*)calloc(oBrush->GetFaceCount(), g_VColUVs_OneSetSize);
  if(pVColUVs==NULL)goto failed_MQUVCopyByVCol;

  DWORD numSetSrc = ReadVColUVs(oBrush, pVColUVs);
  if(numSetSrc==0)goto failed_MQUVCopyByVCol;

  //OutputDebugString("src readed-----------------\n");

  /*FILE *fp = fopen("C:\\tmp\\dbg.bin", "wb");
  if(fp!=NULL)
  {
    fwrite(pVColUVs, g_VColUVs_OneSetSize, numSetSrc, fp);
    fclose(fp);
  }*/
  
  BYTE vcols[13] = {0};
  for(int oi=0;oi<targetObjArr.GetCount();oi++)
  {
    MQObject o = targetObjArr[oi];
    int numFace = o->GetFaceCount();
    //CString s;
    //s.Format("g_VColUVs_OneSetSize==%d\n", g_VColUVs_OneSetSize);
    //OutputDebugString(s);
    //s.Format("sizeof(MQCoordinate)==%d\n", sizeof(MQCoordinate));
    //OutputDebugString(s);
    //o->GetName(s.GetBufferSetLength(255), 255);
    //OutputDebugString(s);
    //s.Format("numFace == %d\n", numFace);
    //OutputDebugString(s);
    for(int fi=0;fi<numFace;fi++)
    {
      int numV = o->GetFacePointCount(fi);
    //s.Format("numV == %d\n", numV);
    //OutputDebugString(s);
      if(numV!=3 && numV!=4)continue;
      memset(vcols, 0, sizeof(vcols));
      for(int j=0;j<numV;j++)
      {
        *((DWORD*)(vcols+3*j)) = o->GetFaceVertexColor(fi, j);
      }
      if(numV==3)vcols[9]=0;
      vcols[12] = numV==3?1:0;
    //  OutputDebugString("-------------\n");
    //DbgVCols(vcols);
      int minIdx = SortVCols(vcols, numV);
    //DbgVCols(vcols);
      int idx = SearchVCols(pVColUVs, numSetSrc, vcols);
      if(idx>=0)
      {
    //DbgVColUVs(pVColUVs+idx*g_VColUVs_OneSetSize);
        SetUVs(o, fi, pVColUVs, idx, minIdx);
      } else OutputDebugString("ERR: vcol not found\n");
    }
  }
  
  if(pVColUVs!=NULL)free(pVColUVs);
  return TRUE;

failed_MQUVCopyByVCol:
  if(pVColUVs!=NULL)free(pVColUVs);
  return FALSE;

}

