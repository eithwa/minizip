#include <iostream>
#include "MyMiniZip.h"
int main()
{
	MyMiniZip unZip;
	/*
	@解?? zip文件包
	*/
	unZip.unZipPackageToLoacal("D:\\test.zip", "d:\\");
	printf_s("Total time taken %d seconds\r\n", unZip.GetCountTime());

	/*
	@ ??文件或目??zip包
	*/
	unZip.CompressToPackageZip("D:\\11111111", "D:\\test.zip");
	printf_s("Total time taken %d seconds\r\n", unZip.GetCountTime());
	system("pause");
	return 0;
}