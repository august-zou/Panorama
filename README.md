Panorama
===========

Panorama说明
------------

-  Panorama 用于图像拼接，全景图生成。
-  原始代码来源于 [Cornell University Computer Vision Course](http://www.cs.cornell.edu/courses/cs6670/2011sp/projects/p2/project2.html)

Panorama安装说明
---------------

1.  依赖说明：Mac 系统中可先安装Homebrew库管理软件（需要安装Ruby环境，Mac中默认已安装),通过命令：

		brew update
		brew install ***'		
	*  Fast Light Toolkit(fltk)
	*  gtk+(or XQuartz-2.7.4) 实际依赖的库文件为X11
	
	
2.  gcc 编译文件

	makefile 命令:
	
		  make

Panorama 使用说明
------------------


usage:

	./Panorama sphrWarp input.tga output.tga f [k1 k2]
	./Panorama alignPair input1.f input2.f matchfile nRANSAC RANSACthresh [sift]
	./Panorama blendPairs pairlist.txt outimg.tga blendWidth
	./Panorama script script.cmd


	   
