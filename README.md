# ESPHackingTest
### 使用C++编写的CS 透视外挂测试程序。（Counter-Strike ESP Hacker Tester using C++ Programming.）
# 主要实现方法：
1.使用CE工具筛选出关键对象在内存中的动态地址。<br />
2.再通过动态地址找到静态地址（偏移值）。<br />
3.拥有这些偏移值后，就可以随时使用memoryapi.h库来获取该对象的指针。<br />
4.获取到这些游戏对象集合的指针后，就可以得到所有的数据，比如游戏角色的坐标系，矩阵对象等等。<br />
5.使用特定算法把三维空间数据转换成二维空间数据（世界空间转换成屏幕矩阵）。<br />
6.调用wingdi.h，WinUser.h库的函数在窗口中生成线和边框。<br />
<br />
通过这几个步骤，就可以制作一个透视外挂，适用于所有射击游戏。<br />
<br />
<br />
<br />
<br />
# 外挂开启前
![SnapShot_03](https://user-images.githubusercontent.com/25366136/55958766-ffa1f080-5c9b-11e9-8db0-d10bcefd740f.jpg)

# 外挂开启后
![SnapShot_01](https://user-images.githubusercontent.com/25366136/55949229-b6927200-5c84-11e9-8555-4f3aa434c54f.jpg)

# 穿墙透视效果
![SnapShot_01](https://user-images.githubusercontent.com/25366136/55949296-d9248b00-5c84-11e9-95e4-f6687ee22863.jpg)
