#include <assert.h>
#include "bpred.h"
#include <math.h>
#include "knob.h"
#include "all_knobs.h"

extern int ghr_len;

//Needs to add knobs for history bits

// FIXED Max 2==>3, and INIT 1==>2
#define BPRED_PHT_CTR_MAX  3
#define BPRED_PHT_CTR_INIT 2

#define BPRED_SAT_INC(X,MAX)  (X<MAX)? (X+1): MAX
#define BPRED_SAT_DEC(X)      X?       (X-1): 0

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bpred * bpred_new(bpred_type type, int hist_len){
  bpred *b = (bpred *) calloc(1, sizeof(bpred));
  
  b->type     = type;
  b->hist_len = hist_len;
  b->pht_entries = (1<<hist_len);

  switch(type){

  case BPRED_NOTTAKEN:
    // nothing to do
    break;

  case BPRED_TAKEN:
    // nothing to do
    break;

  case BPRED_BIMODAL:
    bpred_bimodal_init(b); 
    break;

  case BPRED_GSHARE:
    bpred_gshare_init(b); 
    break;

  default: 
    assert(0);
  }

  return b; // FIXED
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int bpred_access(bpred *b, unsigned int pc){

  switch(b->type){

  case BPRED_NOTTAKEN:
    return 0;

  case BPRED_TAKEN:
    return 1;

  case BPRED_BIMODAL:
    return bpred_bimodal_access(b, pc);

  case BPRED_GSHARE:
    return bpred_gshare_access(b, pc);

  default: 
    assert(0);
  }


}


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void   bpred_update(bpred *b, unsigned int pc, 
		    int pred_dir, int resolve_dir){

  // update the stats
  if(pred_dir == resolve_dir){
    b->okpred++;
  }else{
    b->mispred++;
  }

  
  // update the predictor

  switch(b->type){
    
  case BPRED_NOTTAKEN:
    // nothing to do
    break; 

  case BPRED_TAKEN:
    // nothing to do
    break;

  case BPRED_BIMODAL:
    bpred_bimodal_update(b, pc, pred_dir, resolve_dir);
    break;

  case BPRED_GSHARE:
    bpred_gshare_update(b, pc, pred_dir, resolve_dir);
    break;

  default: 
    assert(0);
  }


}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_bimodal_init(bpred *b){

  b->pht = (unsigned int *) calloc(b->pht_entries, sizeof(unsigned int));

  for(int ii=0; ii< b->pht_entries; ii++){
    b->pht[ii]=BPRED_PHT_CTR_INIT; 
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int   bpred_bimodal_access(bpred *b, unsigned int pc){
  int pht_index = pc % (b->pht_entries);
  int pht_ctr = b->pht[pht_index];

  if(pht_ctr > BPRED_PHT_CTR_MAX/2){
    return 1; // Taken
  }else{
    return 0; // Not-Taken
  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_bimodal_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir){
  int pht_index = pc % (b->pht_entries);
  int pht_ctr = b->pht[pht_index];
  int new_pht_ctr;

  if(resolve_dir==1){
    new_pht_ctr = BPRED_SAT_INC(pht_ctr, BPRED_PHT_CTR_MAX);
  }else{
    new_pht_ctr = BPRED_SAT_DEC(pht_ctr);
  }

  b->pht[pht_index] = new_pht_ctr;  
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_gshare_init(bpred *b)
{

	b->pht = (unsigned int *) calloc(b->pht_entries, sizeof(unsigned int));
	b->ghr=0; // should be done already since using calloc

	for(int i=0; i< b->pht_entries; i++)
	{
	    b->pht[i]=BPRED_PHT_CTR_INIT;
	}

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

int   bpred_gshare_access(bpred *b, unsigned int pc)
{
	// Bitwise XOR of PC with GHR and take the last N bits where N= size of GHR
	  int pht_index = (pc ^ b->ghr) % (unsigned int)pow(2,ghr_len);
	  int pht_ctr = b->pht[pht_index];

	  if(pht_ctr > BPRED_PHT_CTR_MAX/2)
	  {
	    return 1; // Taken
	  }
	  else
	  {
	    return 0; // Not-Taken
	  }
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void  bpred_gshare_update(bpred *b, unsigned int pc, 
			    int pred_dir, int resolve_dir)
{
	int pht_index = (pc ^ b->ghr) % (unsigned int)pow(2, ghr_len);
	int pht_ctr = b->pht[pht_index];
	int new_pht_ctr;

	if(resolve_dir==1)
	{
	   new_pht_ctr = BPRED_SAT_INC(pht_ctr, BPRED_PHT_CTR_MAX);
	}
	else
	{
	   new_pht_ctr = BPRED_SAT_DEC(pht_ctr);
	}

	b->pht[pht_index] = new_pht_ctr;
	b->ghr=(b->ghr << 1) | resolve_dir; // Left-shift GHR by 1 bit and insert actual branch direction value(T or NT)

}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

