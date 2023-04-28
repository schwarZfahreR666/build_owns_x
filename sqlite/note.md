# 一、概论

## 1.基本结构

![sqlite architecture (https://www.sqlite.org/zipvfs/doc/trunk/www/howitworks.wiki)](https://picgozz.oss-cn-beijing.aliyuncs.com/img/202303162014286.gif)

![image-20230316210840121](https://picgozz.oss-cn-beijing.aliyuncs.com/img/202303162108214.png)

### 1.前端

前端主要功能是将用户输入的sql解析为后端引擎执行的字节码。

#### 1.tokenizer

分词

#### 2.parser

语法分析器

#### 3.code generator

字节码生成器

### 2.后端

#### 1.virtual machine

虚拟机将前端生成的字节码作为指令。它可以对一个或多个表或索引执行操作，每个表或索引都存储在称为 B 树的数据结构中。VM 本质上是关于字节码指令类型的switch语句。

#### 2.B-tree

索引数据结构

#### 3.pager

接收用于读取或写入数据页的命令。它负责在数据库文件中以适当的偏移量读取/写入。它还在内存中保留最近访问的页面的缓存，并确定何时需要将这些页面写回磁盘。

#### 4.os interface

针对不同操作系统的api层。

## 5.持久化

![image-20230323153525520](https://picgozz.oss-cn-beijing.aliyuncs.com/img/202303231535642.png)



## 3.游标

用以表示在表中的位置：

- 指向表头
- 指向表尾
- 每一行的访问，并指向下一行



游标的行为：

- 删除游标指向的行
- 修改游标位置
- 根据ID查找表，并把游标指向该行



## 4.B+tree

Leaf node layout

![image-20230324152510162](https://picgozz.oss-cn-beijing.aliyuncs.com/img/202303241525365.png)



Internal node layout

![image-20230327153018549](https://picgozz.oss-cn-beijing.aliyuncs.com/img/202303271530759.png)