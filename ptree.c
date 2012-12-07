/*
MIT License

Copyright (c) 2000 Adrien M. Regimbald

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_TNODES 200
#define MAX_PRI_SIZE 3
#define max		0
#define min		1

#define NOT_PRUNNED 	0
#define PRUNNED  	1

typedef struct tnode_t
{
	uint32_t depth;
	//min or max
	uint32_t type;
	long int alpha;
	long int beta;
	uint32_t num_ch;
	uint32_t num_ch_rem;
	uint32_t status;
	pthread_rwlock_t lock;
	//priority of node
	uint32_t pri_bits; 
	uint64_t pri[MAX_PRI_SIZE];
	//stored pointer to game state
	void* state; 
	//an array of moves
	void* moves;
	//related nodes
	struct tnode_t* parent;
	struct tnode_t* ch;	
	int best_ch;
	int ch_ref;
}tnode;

#define shift_d64(val, num_shift) (((num_shift) == 64) ? (0x0000000000000000) : (val >> num_shift))
#define shift_u64(val, num_shift) (((num_shift) == 64) ? (0x0000000000000000) : (val << num_shift))

//not safe to use with the same address
//function works only for more than 0 zero chunks
inline void shift_down64(uint64_t* n_64_num, uint64_t* p_64_num, uint32_t n_bits)
{
	int i;
	//multiples of 64 bits to shift
	int m_64 = n_bits >> 6;	
	//remainder bits
	int r_bits = n_bits - (m_64*64); 
	//boundary for copying the rest of the chunks
	int b_cpy = MAX_PRI_SIZE - m_64 - 1;
	//printf("b_cpy = %d, m_64 = %d, r_bits = %d\n", b_cpy, m_64, r_bits);
	//set all top m_64 chucks to zero chunks, note MAX_PRI_SIZE>m_64 -> MAX_PRI_SIZE-m_64>0 
	//printf("---\n");
	//printf("n_bits = %d\n", n_bits);
	assert(n_bits <= (MAX_PRI_SIZE*64));
	for(i = MAX_PRI_SIZE-1; i > b_cpy; i--)
	{
		//printf("%d\n", i);
		n_64_num[i] = 0;
	}
	//first chunk is appended with r_bits of zeros
	n_64_num[b_cpy] = 0 | shift_d64(p_64_num[MAX_PRI_SIZE-1], r_bits);
	//chunks from from the MAX_PRI_SIZE - m_64 - 1 + m_64
	//printf("%016llX", p_64_num[i]);
	//printf("---\n");
	for(i = b_cpy-1; i >= 0; i--)
	{
		assert((((i) >= 0 && (i) < MAX_PRI_SIZE) && ((m_64+i) >= 0 && (m_64+i) < MAX_PRI_SIZE)));
		assert(((i+m_64+1) >= 0 && (i+m_64+1) < MAX_PRI_SIZE));
		//printf("%d, i+m_64+1 = %d, i+m_64 = %d\n", i, i+m_64+1, i+m_64);
		//MAX_PRI_SIZE - m_64 - 2 ... 0 [i]
		//MAX_PRI_SIZE - 1 ... m_64 + 1 [i+m_64+1]
		//MAX_PRI_SIZE - 2 ... m_64 	[i+m_64]
		n_64_num[i] = shift_u64(p_64_num[i+m_64+1], (64-r_bits)) | shift_d64(p_64_num[i+m_64], r_bits);
	}
	//printf("---\n");
		
}
//not safe to use with the same address
//function works only for more than 0 zero chunks
inline void shift_up64(uint64_t* n_64_num, uint64_t* p_64_num, uint32_t n_bits)
{
	int i;
	//multiples of 64 bits to shift
	int m_64 = n_bits >> 6;	
	//remainder bits
	int r_bits = n_bits - (m_64*64); 
	//printf("n_bits = %d, r_bits = %d\n", n_bits, r_bits);
	//set all m_64 chucks to zero chunks
	assert(n_bits <= (MAX_PRI_SIZE*64));
	for(i = 0; i < m_64; i++)
	{
		n_64_num[i] = 0;
	}
	//first chunk is appended with r_bits of zeros
	n_64_num[m_64] = shift_u64(p_64_num[0], r_bits) | 0;
	//assume there is atleast one chunk
	//printf("----\n");
	for(i = 1; i < MAX_PRI_SIZE-m_64; i++)
	{
		assert((((i) >= 0 && (i) < MAX_PRI_SIZE) && ((m_64+i) >= 0 && (m_64+i) < MAX_PRI_SIZE)));
		assert(((i-1) >= 0 && (i-1) < MAX_PRI_SIZE));
		//printf("i  = %d, m_64+i = %d, i = %d, i-1 = %d\n", i, m_64+i, i, i-1);
		//[i] 		1 ... MAX_PRI_SIZE - m_64 - 1
		//[m_64+i] 	1+m_64 .. MAX_PRI_SIZE - 1 
		//[i]		1 ... MAX_PRI_SIZE - m_64 - 1
		//[i-1]		0 ... MAX_PRI_SIZE - m_64 - 2
		n_64_num[m_64+i] = shift_u64(p_64_num[i], r_bits) | shift_d64(p_64_num[i-1], (64 - r_bits));
	}
	//printf("----\n");
}
inline int eq(uint64_t* pri_1, uint64_t* pri_2, int n_chunks)
{	
	n_chunks -= 1;
	while(n_chunks >= 0)
	{
		if(pri_1[n_chunks] != pri_2[n_chunks])
			return false;
		n_chunks--;	
	}
	return true;
}
inline int lt(uint64_t* pri_1, uint64_t* pri_2, int n_chunks)
{
	n_chunks -= 1;
	while(n_chunks >= 0)
	{
		if(pri_1[n_chunks] < pri_2[n_chunks])
			return true;
		else if(pri_1[n_chunks] > pri_2[n_chunks])
			return false;

		n_chunks--;	
	}
	return false;
}
inline int gt(uint64_t* pri_1, uint64_t* pri_2, int n_chunks)
{
	n_chunks -= 1;
	while(n_chunks >= 0)
	{
		if(pri_1[n_chunks] > pri_2[n_chunks])
			return true;
		else if (pri_1[n_chunks] < pri_2[n_chunks])
			return false;

		n_chunks--;
	}
	return false;
}

inline tnode* tnode_init()
{
	tnode* tn = (tnode*)malloc(sizeof(tnode));
	tn->status = NOT_PRUNNED;
	pthread_rwlock_init(&(tn->lock), NULL);
	return tn;
} 

inline void tnode_set_pri(tnode* parent, tnode* child, int n_child, int bits_childs)
{
	// 0 <= n_childs < 2^(bits_childs)
	shift_up64(child->pri, parent->pri, bits_childs); 
	child->pri[0] |= n_child;
	// set the number of priority bits in the child
	child->pri_bits = parent->pri_bits + bits_childs;
} 
inline void tnode_free(tnode* tn)
{
	pthread_rwlock_destroy(&(tn->lock));
	free(tn);
}

inline void tnode_print_pri(uint64_t* p_64_num, int n_bits)
{
	int i;
	printf("pri_bits = %d pri = ", n_bits);
	for(i = MAX_PRI_SIZE-1; i >= 0; i--)
		printf("%016llX", p_64_num[i]);
	printf("\n");
}
//implemented only with less than and greater than
int pri_compare(void* tn_va, void* tn_vb)
{
	tnode* tn_a = (tnode*)tn_va;
	tnode* tn_b = (tnode*)tn_vb;	
	int tn_diff, atn_diff;
	uint64_t pri_n[MAX_PRI_SIZE];	

	tn_diff = (tn_a->pri_bits) - (tn_b->pri_bits);
	atn_diff = abs(tn_diff);
	if(tn_diff >= 0)//tn_a has more pri bits so normalize tn_a pri
	{
		shift_up64(pri_n, tn_b->pri, atn_diff);
		if(lt(tn_a->pri, pri_n, MAX_PRI_SIZE))
			return -1;
		else if(gt(tn_a->pri, pri_n, MAX_PRI_SIZE))
			return 1; 
		else 
			if(tn_diff > 0)
				return 1;
			else if(tn_diff == 0)
				return 0;
			else 
				assert(0);
	}
	else //tn_b has more pri bits ...
	{
		shift_up64(pri_n, tn_a->pri, atn_diff);
		if(lt(pri_n, tn_b->pri, MAX_PRI_SIZE))
			return -1;
		else if(gt(pri_n, tn_b->pri, MAX_PRI_SIZE))
			return 1;
		else	//tn_b has more pri bits therefore a is smaller
			return -1;
	}
	return 0;
}

