# 常见per-flow measurement方法

Per-flow measurement指在网络交换机或者路由器测量某个流的某些信息。最典型的是流量测量，即这个流有多少包/字节经过这个交换机



## 每天一个小目标 O_O



## Sampling-based Method

### 1. NetFlow

- flow ID：5-tuple, TOS (Type Of Service) byte, the interface of the router recieved the packet
- 存储方式：在router interface的DRAM里存放一张表，每个entry对应一个流，包含的流信息有，flow ID、timestamp (开始&结束)、packet count、byte count、TCP flags、source network、source AS (Autonomous System)、destination network、destination AS、output interface、next hop router
- Two challenges
  - network traffic流速过快，来不及处理每个packet
  - 收集到的data可能过多，超过了collection server的负载，或者超过了与collection server连接的负载
- Aggregation：通过将一些不重要的数据聚集起来减少exported data
  - 观察：通常不太care end-to-end的流量信息，而关注network/AS之间的流量信息
  - 做法：将复合相同规则的流量信息聚合。比如，相同source AS & destination AS或者相同source network等等
- Sampling：每x个包才更新一次DRAM
- accuracy analysis：所有包大小相同，采样概率为$\frac{1}{x}$，记$c$为NetFlow的counter值，$s$是流的真实大小。
  - 一个流完全没有被测到：$(1-\frac{1}{x})^s$
  - $E(c) = \frac{s}{x}$，因此流的估计大小为$cx$
  - $c$服从二项分布，因此其标准差为$SD[c]=\sqrt{\frac{s}{x}(1-\frac{1}{x})}$，因此估计值的标准差为$\sqrt{sx(1-\frac{1}{x})}$
- 参考网址：https://www.cisco.com/c/en/us/td/docs/ios/12_2/switch/configuration/guide/fswtch_c/xcfnfov.html



## Sketches

### CM sketch

