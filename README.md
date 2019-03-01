# 常见per-flow measurement方法

Per-flow measurement指在网络交换机或者路由器测量某个流的某些信息。最典型的是流量测量，即这个流有多少包/字节经过这个交换机。

<b>Notice:</b> 本说明中的公式在github markdown语法下无法编译。为了更好的阅读，可以将README.md下载后本地查看。


每天一个小目标 O_O！



## Sampling-based Method

### 1. NetFlow (used in Cisco system)

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
  - $E(c) = \frac{s}{x}​$，因此流的估计大小为$cx​$
  - $c$服从二项分布，因此其标准差为$SD[c]=\sqrt{\frac{s}{x}(1-\frac{1}{x})}$，因此估计值的标准差为$\sqrt{sx(1-\frac{1}{x})}$
- 参考网址：https://www.cisco.com/c/en/us/td/docs/ios/12_2/switch/configuration/guide/fswtch_c/xcfnfov.html

### 2. sFlow (published in RFC 3176)

- Sampling：同NetFlow
- 参考网址：https://sflow.org/sFlowOverview.pdf

## Bloom filters

### 1. Bloom filter
- 作用：集合元素存在性查询

- 假阳性分析：假设某集合总共包含n个元素（不重复），bloom filter含有m个比特，使用 k 个哈希函数

  - 某比特为0的概率为：$P_{(b=0)}=(1-\frac{1}{m})^{nk}​$

  - 某不存在元素被误判为存在，即假阳性概率为：$Fpr = (1-P_{(b=0)})^k$

  - 当$m \gg 0, nk \gg 0$，$P_{(b=0)}=(1-\frac{1}{m})^{nk} \approx e^{-\frac{nk}{m}}$，$Fpr \approx (1- e^{-\frac{nk}{m}})^k$

  - 对 $\ln(Fpr) = k \ln{(1-e^{-\frac{nk}{m}})}$ 求导：$\frac{\partial}{\partial k}\ln(Fpr) = \ln (1-e^{-\frac{nk}{m}}) + \frac{\frac{nk}{m}\cdot e^{-\frac{nk}{m}}}{1-e^{-\frac{nk}{m}}}$，令其等于0，可得$e^{-\frac{nk}{m}}=\frac{1}{2}$，即最优k值为：$k=\frac{n}{m}\ln2$

### 2. Counting Bloom filter

## Sketches

### CM sketch

