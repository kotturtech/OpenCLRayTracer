// Sort kernels
// EB Jun 2011

#if CONFIG_USE_VALUE
typedef uint2 data_t;
#define getKey(a) ((a).x)
#define getValue(a) ((a).y)
#define makeData(k,v) ((uint2)((k),(v)))
#else
typedef uint data_t;
#define getKey(a) (a)
#define getValue(a) (0)
#define makeData(k,v) (k)
#endif

// One thread per record
__kernel void Copy(__global const data_t * in,__global data_t * out)
{
  int i = get_global_id(0); // current thread
  out[i] = in[i]; // copy
}

// One thread per record
__kernel void ParallelSelection(__global const data_t * in,__global data_t * out)
{
  int i = get_global_id(0); // current thread
  int n = get_global_size(0); // input size
  data_t iData = in[i];
  uint iKey = getKey(iData);
  // Compute position of in[i] in output
  int pos = 0;
  for (int j=0;j<n;j++)
  {
    uint jKey = getKey(in[j]); // broadcasted
    bool smaller = (jKey < iKey) || (jKey == iKey && j < i); // in[j] < in[i] ?
    pos += (smaller)?1:0;
  }
  out[pos] = iData;
}

#ifndef BLOCK_FACTOR
#define BLOCK_FACTOR 1
#endif
// One thread per record, local memory size AUX is BLOCK_FACTOR * workgroup size keys
__kernel void ParallelSelection_Blocks(__global const data_t * in,__global data_t * out,__local uint * aux)
{
  int i = get_global_id(0); // current thread
  int n = get_global_size(0); // input size
  int wg = get_local_size(0); // workgroup size
  data_t iData = in[i]; // input record for current thread
  uint iKey = getKey(iData); // input key for current thread
  int blockSize = BLOCK_FACTOR * wg; // block size

  // Compute position of iKey in output
  int pos = 0;
  // Loop on blocks of size BLOCKSIZE keys (BLOCKSIZE must divide N)
  for (int j=0;j<n;j+=blockSize)
  {
    // Load BLOCKSIZE keys using all threads (BLOCK_FACTOR values per thread)
    barrier(CLK_LOCAL_MEM_FENCE);
    for (int index=get_local_id(0);index<blockSize;index+=wg)
    {
      aux[index] = getKey(in[j+index]);
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Loop on all values in AUX
    for (int index=0;index<blockSize;index++)
    {
      uint jKey = aux[index]; // broadcasted, local memory
      bool smaller = (jKey < iKey) || ( jKey == iKey && (j+index) < i ); // in[j] < in[i] ?
      pos += (smaller)?1:0;
    }
  }
  out[pos] = iData;
}

// N threads, WG is workgroup size. Sort WG input blocks in each workgroup.
__kernel void ParallelSelection_Local(__global const data_t * in,__global data_t * out,__local data_t * aux)
{
  int i = get_local_id(0); // index in workgroup
  int wg = get_local_size(0); // workgroup size = block size

  // Move IN, OUT to block start
  int offset = get_group_id(0) * wg;
  in += offset; out += offset;

  // Load block in AUX[WG]
  data_t iData = in[i];
  aux[i] = iData;
  barrier(CLK_LOCAL_MEM_FENCE);

  // Find output position of iData
  uint iKey = getKey(iData);
  int pos = 0;
  for (int j=0;j<wg;j++)
  {
    uint jKey = getKey(aux[j]);
    bool smaller = (jKey < iKey) || ( jKey == iKey && j < i ); // in[j] < in[i] ?
    pos += (smaller)?1:0;
  }

  // Store output
  out[pos] = iData;
}

// N threads, WG is workgroup size. Sort WG input blocks in each workgroup.
__kernel void ParallelMerge_Local(__global const data_t * in,__global data_t * out,__local data_t * aux)
{
  int i = get_local_id(0); // index in workgroup
  int wg = get_local_size(0); // workgroup size = block size, power of 2

  // Move IN, OUT to block start
  int offset = get_group_id(0) * wg;
  in += offset; out += offset;

  // Load block in AUX[WG]
  aux[i] = in[i];
  barrier(CLK_LOCAL_MEM_FENCE); // make sure AUX is entirely up to date

  // Now we will merge sub-sequences of length 1,2,...,WG/2
  for (int length=1;length<wg;length<<=1)
  {
    data_t iData = aux[i];
    uint iKey = getKey(iData);
    int ii = i & (length-1);  // index in our sequence in 0..length-1
    int sibling = (i - ii) ^ length; // beginning of the sibling sequence
    int pos = 0;
    for (int inc=length;inc>0;inc>>=1) // increment for dichotomic search
    {
      int j = sibling+pos+inc-1;
      uint jKey = getKey(aux[j]);
      bool smaller = (jKey < iKey) || ( jKey == iKey && j < i );
      pos += (smaller)?inc:0;
      pos = min(pos,length);
    }
    int bits = 2*length-1; // mask for destination
    int dest = ((ii + pos) & bits) | (i & ~bits); // destination index in merged sequence
    barrier(CLK_LOCAL_MEM_FENCE);
    aux[dest] = iData;
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  // Write output
  out[i] = aux[i];
}

// N threads, WG is workgroup size. Sort WG input blocks in each workgroup.
__kernel void ParallelBitonic_Local(__global const data_t * in,__global data_t * out,__local data_t * aux)
{
  int i = get_local_id(0); // index in workgroup
  int wg = get_local_size(0); // workgroup size = block size, power of 2

  // Move IN, OUT to block start
  int offset = get_group_id(0) * wg;
  in += offset; out += offset;

  // Load block in AUX[WG]
  aux[i] = in[i];
  barrier(CLK_LOCAL_MEM_FENCE); // make sure AUX is entirely up to date

  // Loop on sorted sequence length
  for (int length=1;length<wg;length<<=1)
  {
    bool direction = ((i & (length<<1)) != 0); // direction of sort: 0=asc, 1=desc
    // Loop on comparison distance (between keys)
    for (int inc=length;inc>0;inc>>=1)
    {
      int j = i ^ inc; // sibling to compare
      data_t iData = aux[i];
      uint iKey = getKey(iData);
      data_t jData = aux[j];
      uint jKey = getKey(jData);
      bool smaller = (jKey < iKey) || ( jKey == iKey && j < i );
      bool swap = smaller ^ (j < i) ^ direction;
      barrier(CLK_LOCAL_MEM_FENCE);
      aux[i] = (swap)?jData:iData;
      barrier(CLK_LOCAL_MEM_FENCE);
    }
  }

  // Write output
  out[i] = aux[i];
}
__kernel void ParallelBitonic_Local_Optim(__global const data_t * in,__global data_t * out,__local data_t * aux)
{
  int i = get_local_id(0); // index in workgroup
  int wg = get_local_size(0); // workgroup size = block size, power of 2

  // Move IN, OUT to block start
  int offset = get_group_id(0) * wg;
  in += offset; out += offset;

  // Load block in AUX[WG]
  data_t iData = in[i];
  aux[i] = iData;
  barrier(CLK_LOCAL_MEM_FENCE); // make sure AUX is entirely up to date

  // Loop on sorted sequence length
  for (int length=1;length<wg;length<<=1)
  {
    bool direction = ((i & (length<<1)) != 0); // direction of sort: 0=asc, 1=desc
    // Loop on comparison distance (between keys)
    for (int inc=length;inc>0;inc>>=1)
    {
      int j = i ^ inc; // sibling to compare
      data_t jData = aux[j];
      uint iKey = getKey(iData);
      uint jKey = getKey(jData);
      bool smaller = (jKey < iKey) || ( jKey == iKey && j < i );
      bool swap = smaller ^ (j < i) ^ direction;
      iData = (swap)?jData:iData; // update iData
      barrier(CLK_LOCAL_MEM_FENCE);
      aux[i] = iData;
      barrier(CLK_LOCAL_MEM_FENCE);
    }
  }

  // Write output
  out[i] = iData;
}

// N threads
__kernel void ParallelBitonic_A(__global const data_t * in,__global data_t * out,int inc,int dir)
{
  int i = get_global_id(0); // thread index
  int j = i ^ inc; // sibling to compare

  // Load values at I and J
  data_t iData = in[i];
  uint iKey = getKey(iData);
  data_t jData = in[j];
  uint jKey = getKey(jData);

  // Compare
  bool smaller = (jKey < iKey) || ( jKey == iKey && j < i );
  bool swap = smaller ^ (j < i) ^ ((dir & i) != 0);

  // Store
  out[i] = (swap)?jData:iData;
}

// N/2 threads
__kernel void ParallelBitonic_B_test(__global const data_t * in,__global data_t * out,int inc,int dir)
{
  int t = get_global_id(0); // thread index
  int low = t & (inc - 1); // low order bits (below INC)
  int i = (t<<1) - low; // insert 0 at position INC
  int j = i | inc; // insert 1 at position INC

  // Load values at I and J
  data_t iData = in[i];
  uint iKey = getKey(iData);
  data_t jData = in[j];
  uint jKey = getKey(jData);

  // Compare
  bool smaller = (jKey < iKey) || ( jKey == iKey && j < i );
  bool swap = smaller ^ ((dir & i) != 0);

  // Store
  out[i] = (swap)?jData:iData;
  out[j] = (swap)?iData:jData;
}

#define ORDER(a,b) { bool swap = reverse ^ (getKey(a)<getKey(b)); data_t auxa = a; data_t auxb = b; a = (swap)?auxb:auxa; b = (swap)?auxa:auxb; }

// N/2 threads
__kernel void ParallelBitonic_B2(__global data_t * data,int inc,int dir)
{
  int t = get_global_id(0); // thread index
  int low = t & (inc - 1); // low order bits (below INC)
  int i = (t<<1) - low; // insert 0 at position INC
  bool reverse = ((dir & i) == 0); // asc/desc order
  data += i; // translate to first value

  // Load
  data_t x0 = data[  0];
  data_t x1 = data[inc];

  // Sort
  ORDER(x0,x1)

  // Store
  data[0  ] = x0;
  data[inc] = x1;
}

// N/4 threads
__kernel void ParallelBitonic_B4(__global data_t * data,int inc,int dir)
{
  inc >>= 1;
  int t = get_global_id(0); // thread index
  int low = t & (inc - 1); // low order bits (below INC)
  int i = ((t - low) << 2) + low; // insert 00 at position INC
  bool reverse = ((dir & i) == 0); // asc/desc order
  data += i; // translate to first value

  // Load
  data_t x0 = data[    0];
  data_t x1 = data[  inc];
  data_t x2 = data[2*inc];
  data_t x3 = data[3*inc];

  // Sort
  ORDER(x0,x2)
  ORDER(x1,x3)
  ORDER(x0,x1)
  ORDER(x2,x3)

  // Store
  data[    0] = x0;
  data[  inc] = x1;
  data[2*inc] = x2;
  data[3*inc] = x3;
}

#define ORDERV(x,a,b) { bool swap = reverse ^ (getKey(x[a])<getKey(x[b])); \
      data_t auxa = x[a]; data_t auxb = x[b]; \
      x[a] = (swap)?auxb:auxa; x[b] = (swap)?auxa:auxb; }
#define B2V(x,a) { ORDERV(x,a,a+1) }
#define B4V(x,a) { for (int i4=0;i4<2;i4++) { ORDERV(x,a+i4,a+i4+2) } B2V(x,a) B2V(x,a+2) }
#define B8V(x,a) { for (int i8=0;i8<4;i8++) { ORDERV(x,a+i8,a+i8+4) } B4V(x,a) B4V(x,a+4) }
#define B16V(x,a) { for (int i16=0;i16<8;i16++) { ORDERV(x,a+i16,a+i16+8) } B8V(x,a) B8V(x,a+8) }

// N/8 threads
__kernel void ParallelBitonic_B8(__global data_t * data,int inc,int dir)
{
  inc >>= 2;
  int t = get_global_id(0); // thread index
  int low = t & (inc - 1); // low order bits (below INC)
  int i = ((t - low) << 3) + low; // insert 000 at position INC
  bool reverse = ((dir & i) == 0); // asc/desc order
  data += i; // translate to first value

  // Load
  data_t x[8];
  for (int k=0;k<8;k++) x[k] = data[k*inc];

  // Sort
  B8V(x,0)

  // Store
  for (int k=0;k<8;k++) data[k*inc] = x[k];
}

// N/16 threads
__kernel void ParallelBitonic_B16(__global data_t * data,int inc,int dir)
{
  inc >>= 3;
  int t = get_global_id(0); // thread index
  int low = t & (inc - 1); // low order bits (below INC)
  int i = ((t - low) << 4) + low; // insert 0000 at position INC
  bool reverse = ((dir & i) == 0); // asc/desc order
  data += i; // translate to first value

  // Load
  data_t x[16];
  for (int k=0;k<16;k++) x[k] = data[k*inc];

  // Sort
  B16V(x,0)

  // Store
  for (int k=0;k<16;k++) data[k*inc] = x[k];
}

// N/2 threads, AUX[2*WG]
__kernel void ParallelBitonic_C2_pre(__global data_t * data,int inc,int dir,__local data_t * aux)
{
  int t = get_global_id(0); // thread index

  // Terminate the INC loop inside the workgroup
  for ( ;inc>0;inc>>=1)
  {
    int low = t & (inc - 1); // low order bits (below INC)
    int i = (t<<1) - low; // insert 0 at position INC
    bool reverse = ((dir & i) == 0); // asc/desc order

    barrier(CLK_GLOBAL_MEM_FENCE);

    // Load
    data_t x0 = data[i];
    data_t x1 = data[i+inc];

    // Sort
    ORDER(x0,x1)

    barrier(CLK_GLOBAL_MEM_FENCE);

    // Store
    data[i] = x0;
    data[i+inc] = x1;
  }
}

// N/2 threads, AUX[2*WG]
__kernel void ParallelBitonic_C2(__global data_t * data,int inc0,int dir,__local data_t * aux)
{
  int t = get_global_id(0); // thread index
  int wgBits = 2*get_local_size(0) - 1; // bit mask to get index in local memory AUX (size is 2*WG)

  for (int inc=inc0;inc>0;inc>>=1)
  {
    int low = t & (inc - 1); // low order bits (below INC)
    int i = (t<<1) - low; // insert 0 at position INC
    bool reverse = ((dir & i) == 0); // asc/desc order
    data_t x0,x1;

    // Load
    if (inc == inc0)
    {
      // First iteration: load from global memory
      x0 = data[i];
      x1 = data[i+inc];
    }
    else
    {
      // Other iterations: load from local memory
      barrier(CLK_LOCAL_MEM_FENCE);
      x0 = aux[i & wgBits];
      x1 = aux[(i+inc) & wgBits];
    }

    // Sort
    ORDER(x0,x1)

    // Store
    if (inc == 1)
    {
      // Last iteration: store to global memory
      data[i] = x0;
      data[i+inc] = x1;
    }
    else
    {
      // Other iterations: store to local memory
      barrier(CLK_LOCAL_MEM_FENCE);
      aux[i & wgBits] = x0;
      aux[(i+inc) & wgBits] = x1;
    }
  }
}

// N/4 threads, AUX[4*WG]
__kernel void ParallelBitonic_C4_0(__global data_t * data,int inc0,int dir,__local data_t * aux)
{
  int t = get_global_id(0); // thread index
  int wgBits = 4*get_local_size(0) - 1; // bit mask to get index in local memory AUX (size is 4*WG)

  for (int inc=inc0>>1;inc>0;inc>>=2)
  {
    int low = t & (inc - 1); // low order bits (below INC)
    int i = ((t - low) << 2) + low; // insert 00 at position INC
    bool reverse = ((dir & i) == 0); // asc/desc order
    data_t x[4];
    
    // Load
    if (inc == inc0>>1)
    {
      // First iteration: load from global memory
      for (int k=0;k<4;k++) x[k] = data[i+k*inc];
    }
    else
    {
      // Other iterations: load from local memory
      barrier(CLK_LOCAL_MEM_FENCE);
      for (int k=0;k<4;k++) x[k] = aux[(i+k*inc) & wgBits];
    }

    // Sort
    B4V(x,0);

    // Store
    if (inc == 1)
    {
      // Last iteration: store to global memory
      for (int k=0;k<4;k++) data[i+k*inc] = x[k];
    }
    else
    {
      // Other iterations: store to local memory
      barrier(CLK_LOCAL_MEM_FENCE);
      for (int k=0;k<4;k++) aux[(i+k*inc) & wgBits] = x[k];
    }
  }
}

__kernel void ParallelBitonic_C4(__global data_t * data,int inc0,int dir,__local data_t * aux)
{
  int t = get_global_id(0); // thread index
  int wgBits = 4*get_local_size(0) - 1; // bit mask to get index in local memory AUX (size is 4*WG)
  int inc,low,i;
  bool reverse;
  data_t x[4];

  // First iteration, global input, local output
  inc = inc0>>1;
  low = t & (inc - 1); // low order bits (below INC)
  i = ((t - low) << 2) + low; // insert 00 at position INC
  reverse = ((dir & i) == 0); // asc/desc order
  for (int k=0;k<4;k++) x[k] = data[i+k*inc];
  B4V(x,0);
  for (int k=0;k<4;k++) aux[(i+k*inc) & wgBits] = x[k];
  barrier(CLK_LOCAL_MEM_FENCE);

  // Internal iterations, local input and output
  for ( ;inc>1;inc>>=2)
  {
    low = t & (inc - 1); // low order bits (below INC)
    i = ((t - low) << 2) + low; // insert 00 at position INC
    reverse = ((dir & i) == 0); // asc/desc order
    for (int k=0;k<4;k++) x[k] = aux[(i+k*inc) & wgBits];
    B4V(x,0);
    barrier(CLK_LOCAL_MEM_FENCE);
    for (int k=0;k<4;k++) aux[(i+k*inc) & wgBits] = x[k];
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  // Final iteration, local input, global output, INC=1
  i = t << 2;
  reverse = ((dir & i) == 0); // asc/desc order
  for (int k=0;k<4;k++) x[k] = aux[(i+k) & wgBits];
  B4V(x,0);
  for (int k=0;k<4;k++) data[i+k] = x[k];
}
