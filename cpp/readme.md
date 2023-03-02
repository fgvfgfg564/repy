# Range Encoder C++实现

test.cpp为测试程序，提供使用样例
range_encoder.cpp内为算法详细过程

encode：输入概率分布表格（每个位置共用同一个先验分布）和token串，输出码流
decode：输入概率分布表格（与编码端一致）和码流，输出解码后的token串