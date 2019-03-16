# 常见Sketch&BloomFilter算法

Per-flow measurement指在网络交换机或者路由器测量某个流的某些信息。最典型的是流量测量，即这个流有多少包/字节经过这个交换机。

<b>Notice:</b> 本说明中的公式在github markdown语法下无法编译。为了更好的阅读，可以将README.md下载后本地查看。

每天一个小目标 O_O！

[Sampling-based Method](#sampling-based-method)

[Bloom filters](#bloom-filters)

[Sketches](#sketches)

[Others](#others)




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
- 作用：单集合元素存在性查询

- 假阳性分析：假设某集合总共包含n个元素（不重复），bloom filter含有m个比特，使用 k 个哈希函数

  - 某比特为0的概率为：$P_{(b=0)}=(1-\frac{1}{m})^{nk}​$

  - 某不存在元素被误判为存在，即假阳性概率为：$Fpr = (1-P_{(b=0)})^k$

  - 当$m \gg 0, nk \gg 0$，$P_{(b=0)}=(1-\frac{1}{m})^{nk} \approx e^{-\frac{nk}{m}}$，$Fpr \approx (1- e^{-\frac{nk}{m}})^k$

  - 对 $\ln(Fpr) = k \ln{(1-e^{-\frac{nk}{m}})}$ 求导：$\frac{\partial}{\partial k}\ln(Fpr) = \ln (1-e^{-\frac{nk}{m}}) + \frac{\frac{nk}{m}\cdot e^{-\frac{nk}{m}}}{1-e^{-\frac{nk}{m}}}$，令其等于0，可得$e^{-\frac{nk}{m}}=\frac{1}{2}$，即最优k值为：$k=\frac{n}{m}\ln2$

### 2. Counting Bloom filter

- 作用：多重集合元素查询

- 做法：某集合s中的元素可以重复，因此把每个bit换成counter就行了

### 3. Summary Cache
- 作用：多集合元素查询（每个集合中元素不重复，且集合之间没有交集，查询一个元素属于哪个集合）

- 做法：每个集合对应一个bloom filter

- 缺点：查询时需要访存次数太多

### 4. Coded Bloom Filter

- 作用：多集合元素查询
- 做法：每个集合对应一个ID（序号），因此可以用一个bloom filter对应ID的一个bit
- 缺点：各个ID所包含的1的数量不一样，插入速度变慢
- 优化：把ID扩张一倍，后面补个反码

### 5. Combinatorial Bloom Filter

- 作用：多集合元素查询
- 做法：只用一个很大的bloom filter，但是使用不同的hash function组来表示不同的集合
- 缺点：插入速度慢

### 6. kBF

- 作用：key-value的静态插入、查询
- 数据结构：
  - 一个cell数组
  - 每个cell包括一个counter part和coding part
- 做法：
  - value -> encoding: 
    - encoding要求：
      - 不同value的encoding不同
      - 任意两个encoding的异或值都是独特的
    - 假设有n个不同value，给第一个value分配encoding为1
    - 对于之后的value，encoding不断+1
    - 如果第k个value发现，它的encoding与之前k-1个value的encoding的异或值已经出现过（可以使用bloom filter），那么第k个value的encoding+1，并继续检查是否满足要求
  - encoding insertion (k次hash)：
    - counter部分+1
    - encoding通过异或的方式插入coding part
  - query：通过hash得到k个cell后，希望还原出encoding
    - 如果存在counter=1的cell，done
    - 如果存在几个counter=2的cell，用O(n)时间复杂度的方法还原两个encoding，并判断共同出现的encoding
    - 如果只有counter>2的cell：
      - 先用O(n)的时间复杂度得到一个，再用o(n)的时间复杂度得到另一个
      - 结果不保证准确
  - encoding->value:
    - 由value->encoding的分配过程可以看出，encoding的分配是按encoding值的大小升序分配的（由此可以记录下一个encoding->value的list），所以可以使用二分查找的方法得到对应的value
    - 原文中提到了另一种更快的方法，没仔细看，就不说了

### 7. Bloomtree

- 作用：多集合查询（key，value=setID）
- 特点：
  - 静态的，不支持更新，可以通过改成counting bloom tree来支持
- 结构：
  - 每个节点都是一个bloom filter
    - 内部节点有多组hash function，每组对应着一个子节点
    - 叶子结点只有一组hash function，用来记录某个key是否存在
- 插入：
  - 根据value逐层选择一组hash函数，对key进行哈希，记录在node（bloom filter）中
- 查询：
  - 每一层，使用这个node的所有哈希函数对key做哈希，查看是否在这个node里面出现过
  - 直到一个leaf node，如果leaf node也出现过，那么回答这个leaf node对应的value

### 8. Bloomier Filter

- 希望解决的问题：
  - 给定定义域：$D=\{0, 1, \ldots, N-2\}$
  - 定义域的一个子集：$S=\{t_1, t_2, \ldots, t_n\}$
  - 值域：$R=\{null, 1, 2, \ldots, |R|-1\}$
  - 希望建立一个$D\rightarrow R$的映射：
    - $for ~~1\leq i \leq n,~~~~ f(t_i) = v_i$
    - $for ~~x\in D/S, ~~~~f(x)=null$
  - $t_i \rightarrow v_i$的映射关系由具体任务决定：比如多重集合查询，t是元素，v是集合id
  - 支持更新，但不支持插入
- 一些定义：
  - 对于定义域中的一个元素t，对其做k次hash得到$(h_1, h_2,\ldots, h_k)$,称这些哈希值为t的neighborhood $N(t)$
  - 令$\pi$表示S上的一个全序关系：
    - 在$\pi$规则下，$S_i >_{\pi} S_j$当且仅当$i>j$
  - 称$\tau$服从关系$(S, \pi, N)$,如果满足以下条件：
    - 若$t\in S​$，则$\tau (t) \in N(t)​$
    - 若$t_i >_{\pi} t_j$，则$\tau(t_i) \notin N(t_j)$
  - singleton：如果某个位置只被S中一个元素映射到过，这个位置就是singleton
  - TWEAK(t, S, HASH)：t的neighborhood $N(t)=(h_1, h_2, \ldots, h_k)$中，所有singleton中最小的那个下标（也就是哪个哈希函数最先映射到singleton）
- 给定S和k个哈希函数，如何得到想要的$\pi$和$\tau​$：
  - 首先找到有TWEAK的元素集合E，因此它们可以满足条件。把E中元素放在ordering最后（也就是E中元素在$\pi$关系下最大）
  - 剩下的元素称为H：继续递归查找$\pi$和$\tau$
  - 可能会失败
- 构建Bloomier filter：
  - 不断尝试，找到想要的$\pi$和$\tau$
  - 对S中任一元素$t$，找到位置$\tau(t)$，把对应哈希函数的编码用异或的方式记录下来（在table 1中）
  - 在table2的位置$\tau(t)$，记录t对应的value
- 查询/更新：
  - 把哈希位置的值全部和mask全部异或起来，解码得到l
  - 查看l是不是key对应的那个l
  - 如果是，返回/修改table2中的value
  - 如果不是，返回这个key不存在

### 9. Cuckoo Filter

- 作用：单集合元素查询
- 做法：filter有bucket array构成，每个bucket包含多个entry，每个entry存放一个partial key
  - 先由key计算partial key：$f​$
  - 计算两个候选位置：$pos_1 = hash(key)$ 和 $pos_2 = hash(key)~XOR ~hash(f)$
  - 插入：如果有空位，就插入；否则，踢掉一个插进去，并把踢掉的那个找另一个候选位置，放进去
  - 查询：查$pos_1$和$pos_2$位置是不是有partial key相等的entry
  - 删除：删掉候选位置与$f$ 相等的entry你们 
### 10. Shifting Bloom Filter

- 作用：多集合元素查询
- insertion：
  - k次哈希，定位到k个bit
  - 第j个集合，那么就把第k个bit之后的offset为j的bits置为1
- query：
  - 和平常的bloom filter一样

### 11. Invertible Bloom Filter

- 作用：
- 数据结构：
  - k个哈希
  - a table of m cells，每个cell包括：
    - count part：映射到这个cell的元素个数
    - keySum：映射到这儿的key的和
    - valueSum：映射到这儿的value的和
- 插入、删除：用哈希函数映射到k个cell，然后按照上面的定义更新这些part就行了
- 查找：先找到k个哈希的cell
  - 如果有count为0的cell，返回null
  - 如果有count为1的cell
    - 如果keySum匹配，返回valueSum
    - 否则返回null
  - 如果所有count都大于1，返回“not found”
- 优势之处：可以list out存在IBLT中的所有key-value pair
  - 先找一个count为1的cell，列出这个cell里的key和value，然后把与其相关的所有cell的值减去key-value
  - 重复上述步骤，直到没有count为1的cell
  - 可能不能把所有的key-value pair输出

## Sketches

### 1. CM sketch

- 作用：频率查询
- 插入：映射到k个counter，每个counter+1
- 查询：映射到k个counter，返回最小counter的值
### 2. CU sketch
- 作用：频率查询
- 插入：映射到k个counter，最小的一个或者多个counter+1
- 查询：映射到k个counter，返回最小counter的值
### 3. Count sketch

- 作用：频率查询
- 插入：映射到k个counter，每个counter等概率+1或者-1
- 查询：映射到k个counter，返回counter绝对值的中位数

### 4. CSM sketch

- 数据结构：
  - 一个包含m个counter的数组
  - k个哈希函数
- 插入：与CM类似，但事实随机选取一个counter+1
- 查询：$\hat{s} = \sum_{i=0}^{k-1}C[h_i(f)] - k\frac{n}{m}$
  - 前面这一块是所有对应hash counter的和，即为真实值+噪音
  - 后面为噪音的期望值（近似值）

### 5. Pyramid sketch

- 数据结构：由多层counter数组构成
  - 上层是下层的一半大小，构成树结构
  - 第一层的counter全部用来记录frequency
  - 之后层的counter的两个bit用来记录左子节点和右子节点是否溢出，剩下的bit用来记录frequency
- 插入过程：
  - 可以使用count、CM、CU、CSM的方式
  - 如发生溢出，则用进位的方式向上层记录
- 删除过程：
  - 可以使用count、CM、CU、CSM的方式
  - 如需要借位，则向下层借（进位的方式）
- 查询过程：
  - 按照进位的方式查询即可
- 好处：因为实际流量中frequency较小的流比较多，因此低层可以使用较小的counter节省空间



## Others
### 1. Space-Saving

- 作用：finding heavy items (items with large frequency)
- 插入算法简单介绍：
  - 若数据结构未满，直接插入
  - 若满了，则替换value最小的元素，并让value+1
- 数据结构：
  - 一个哈希表：key -> key_node的指针
  - 一个value node list，是双向链表，node代表的值从小到大排布
  - 每个value node连着一串key node
    - key node之间用双向链表连接
    - 每个key node还存着一个指针指向value node
- 具体的插入（key）过程：
  - 如果在哈希表中找到key，执行下面的increment操作
    - 将对应的key node从所在的value node中移除
      - 如果移除后list空了，那么将这个value node从value node list中移除
    - 查看原来value node中的下一个value node
      - 如果新node的value = old node的值+1，那么在这个value node所在的key node list中插入这个key
      - 否则，新建一个value node，将值职位old value+1，并将这个key插入到这个value ndoe
  - 如果没有找到这个key：
    - 将v1（最小值的value node）中一个key node的key替换成插入的key，并将这个key node从v1中移除
    - 执行上面的increment操作

### 2. UnivMon

- 处理对象：n个不同的元素，总共m个包，$f_i$表示第i个元素的频率
- 目标是回答G-sum：$\sum g(f_i)$，其中g是单调的，并且以 $f_i^2$ 为upper bound
- 数据结构：$H=\log(n)$个count sketch，按照类似pyramid sketch那样叠加起来
- （online）插入过程：
  - 哈希H次（哈希值落在{0, 1}之间）
  - 如果发现1～j个的哈希值都是1（这样的最大的j），那么插入第1～j个count sketch
- （offline）查询过程：
  - 从数据结构中的每一层count sketch读出heavy hitter集合（命名为$Q_j, 1\leq j \leq \log(n)$, $w_j(i)$表示$Q_j$中的第i个heavy hitter)
  - 对这些heavy hitter作g()操作
  - 计算顶层的G-sum：$Y_{\log(n)}=\sum_{i} g(w_{\log(n)}(i))$
  - 对于接下来的层次：
    - $Y_j = 2Y_{j+1} + \sum_{i \in Q_j}(1 - 2 h_{j+1}(i))(g(w_j(i))$
    - 第一项：G-sum的2倍
    - 后一项：
      - 如果$Q_j(i)$不在第j+1层出现，那么$h_{j+1}(i)=0$，所以后面一项代表加上$Q_j(i)$的counter值
      - 如果$Q_j(i)$在第j+1层出现，那么$h_{j+1}(i)=1$，所以后面一项代表减去$Q_j(i)$的counter值，因此保证了G-sum是1倍的
        - 这里有个隐含的东西：出现在第j+1层的必定出现在第j层，所以第j+1层的G-sum肯定会全部被减一遍
      - 这样做的好处是方便流水化
  - task：
    - heavy hiiter：直接用count sketch做
    - entropy

### 3. FlowRadar 

- 作用：key-value的流量测量任务
- 数据结构很简单：一个bloom filter + 一个counting table（类似IBLT的东西）
  - counting table中的每个cell包含3哥域：
    - FlowXOR：映射到这里面的flow ID的异或值
    - FlowCount：映射到这里面的流个数
    - packetCount：映射到这里的包个数
- 插入key：
  - 检查bloom filter，key是否出现过？
    - 如果出现过，那么在counting table中映射到k个位置，执行packetCount++
    - 否则，先将key插入bloom filter，再映射到counting table中的k个位置，
      - 将key异或进入FlowXOR
      - FlowCount++
      - packetCount++
  - 解码过程（就是IBLT的list out操作）
    -  先找FlowCount=1的cell，读出里面的key和value，然后从与这个key相关的cell删除信息
    - 继续上一步
  - Network-Wide decoding：
    - FlowDecode across switches：	
      - 现在switche内部作decode
      - 然后比较两个相邻两个switch的解码出来的元素集合
        - S1表示一个交换机的解码结果， S2表示另一个交换机的解码结果
        - 查看S1-S2中的元素是不是在switch 2的bloom filter中出现
          - 如果出现了，把这个元素加入S2的结果，并且更新对应的FlowXOR、FlowCount、PacketCount
        - 再对S2-S1同样做一次
    - CounterDecode at a single switch:
      - 通过上面的FlowDecode across switches，大概知道哪些counter里有哪些key，且知道这些key对应的流的包总量
      - 假设有m个counter，n个flow，那么可以得到一个包含n个变量、m个等式的方程组
      - 然后用matlab等工具来解这些东西，最终的到key对应的packet num

### 4. SketchVisor

- 一个集成多种sketch algorithm for various measurement tasks的system
- overview：
  - data plane分为normal path和fast path：
    - 数据进入data plane后，首先判断一个FIFO buffer是否已满
      - 如果没满，则走normal path，由内部各种算法来处理
      - 如果满了，则走**fast path**
  - control plane：
    - 从各地收集数据
    - 将各地收回的数据进行一定的修复（a recover algorithm based on Compressive Sensing）
- Fast Path
  - key idea：
    - 假设进入fast path的流也是长尾分布的 => 应注重收集large flow的信息
    - 同时小流信息也很重要 => 不单独为记录每个小流的信息，而是记录它们的统计信息
    - 总而言之，就是设计一个top-k算法（具体算法还是直接看论文吧，论文讲的很清楚）
- recovery algorithm:
  - 已知的信息：
    - 将所有sketch加在一起，得到一个N（就是对应位置的counter相加）
      - N是明显不准确的，因为走fast path的小流的信息都被扔掉了
    - 所有的top-k流组成一个hash表H
      - H的误差只与选择的算法本身有关
    - 记录流的总字节数V
      - 这个可以完全准确
  - 补全N => 矩阵补全的问题
    - 使用compressive sensing来补全N
    - 具体的可以看论文s

