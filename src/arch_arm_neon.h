/* Copyright 2022 Sipeed Technology Co., Ltd. All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "stdlib.h"
#include "stdint.h"
#include "math.h"
#include "tm_port.h"

#include "arm_neon.h"
//ARM NEON instruction acceleration
//https://developer.arm.com/architectures/instruction-sets/intrinsics/
//https://arm-software.github.io/acle/neon_intrinsics/advsimd.html
//https://blog.csdn.net/fengbingchun/article/details/38085781
//https://blog.csdn.net/u010580016/article/details/110943019

/********************************** TM_MDL_INT8 ***********************************************/
#if TM_MDL_TYPE==TM_MDL_INT8
#define PARALLEL_CNT 8
TM_INLINE void tm_dot_prod(mtype_t* sptr, mtype_t* kptr,uint32_t size, sumtype_t* result){
    uint32_t cnt;                       
    sumtype_t sum = 0;                                            
    cnt = size/PARALLEL_CNT;

    int32x4_t _sum = vdupq_n_s32(0);
    for(int i = 0; i < cnt; i++) {
        int8x8_t s8, k8;
        s8 = vld1_s8(sptr);//8 value
        k8 = vld1_s8(kptr);
        _sum = vmlal_s16(_sum, vget_low_s16(vmovl_s8(s8)), vget_low_s16(vmovl_s8(k8)));
        _sum = vmlal_s16(_sum, vget_high_s16(vmovl_s8(s8)),vget_high_s16( vmovl_s8(k8)));
        sptr += PARALLEL_CNT;
        kptr += PARALLEL_CNT;
    }
    int32_t _sums[4];
    vst1q_s32(_sums, _sum);
    for(int i = 0; i < 4; i++) 
        sum += _sums[i];

    cnt = size % PARALLEL_CNT;
    while (cnt > 0U){
        sum += (int32_t) ((int16_t) *sptr++ * *kptr++);
        cnt--;
    }

    *result = sum;
    return;
}

TM_INLINE  void tm_dot_prod_pack2(mtype_t* sptr, mtype_t* kptr, uint32_t size, sumtype_t* result)
{ 
    uint32_t cnt;                       
    sumtype_t sum0 = 0;  
    sumtype_t sum1 = 0;           
    mtype_t* kptr0 = kptr;
    mtype_t* kptr1 = kptr+size;                                
    cnt = size/PARALLEL_CNT;

    int32x4_t _sum0 = vdupq_n_s32(0);
    int32x4_t _sum1 = vdupq_n_s32(0);
    for(int i = 0; i < cnt; i++) {
        int8x8_t s8, k80,k81;
        s8 = vld1_s8(sptr);//8 value
        k80 = vld1_s8(kptr0);
        k81 = vld1_s8(kptr1);
        _sum0 = vmlal_s16(_sum0, vget_low_s16(vmovl_s8(s8)), vget_low_s16(vmovl_s8(k80)));
        _sum1 = vmlal_s16(_sum1, vget_low_s16(vmovl_s8(s8)), vget_low_s16(vmovl_s8(k81)));
        _sum0 = vmlal_s16(_sum0, vget_high_s16(vmovl_s8(s8)),vget_high_s16( vmovl_s8(k80)));
        _sum1 = vmlal_s16(_sum1, vget_high_s16(vmovl_s8(s8)),vget_high_s16( vmovl_s8(k81)));
        sptr += PARALLEL_CNT;
        kptr0 += PARALLEL_CNT;
        kptr1 += PARALLEL_CNT;
    }
    int32_t _sums0[4];
    int32_t _sums1[4];
    vst1q_s32(_sums0, _sum0);
    vst1q_s32(_sums1, _sum1);
    for(int i = 0; i < 4; i++) {
        sum0 += _sums0[i];
        sum1 += _sums1[i];
    }

    cnt = size % PARALLEL_CNT;
    while (cnt > 0U){
        sum0 += (int32_t) ((int16_t) *sptr * *kptr0++);
        sum1 += (int32_t) ((int16_t) *sptr++ * *kptr1++);
        cnt--;
    }

    result[0] = sum0;
    result[1] = sum1;
    return;
}

TM_INLINE void tm_dot_prod_gap_3x3x1(mtype_t* sptr, mtype_t* kptr, uint32_t* k_oft, sumtype_t* result)
{
    *result = sptr[k_oft[0]]*kptr[0] + sptr[k_oft[1]]*kptr[1] + sptr[k_oft[2]]*kptr[2] + \
        sptr[k_oft[3]]*kptr[3] + sptr[k_oft[4]]*kptr[4] + sptr[k_oft[5]]*kptr[5] + \
        sptr[k_oft[6]]*kptr[6] + sptr[k_oft[7]]*kptr[7] + sptr[k_oft[8]]*kptr[8] ;
    return;                  
}

TM_INLINE void tm_dot_prod_3x3x1(mtype_t* sptr, mtype_t* kptr, sumtype_t* result)
{
    *result = sptr[0]*kptr[0] + sptr[1]*kptr[1] + sptr[2]*kptr[2] + \
        sptr[3]*kptr[3] + sptr[4]*kptr[4] + sptr[5]*kptr[5] + \
        sptr[6]*kptr[6] + sptr[7]*kptr[7] + sptr[8]*kptr[8] ;
    return;
}

/********************************** TM_MDL_FP32 ***********************************************/
#elif TM_MDL_TYPE==TM_MDL_FP32
#define PARALLEL_CNT 4
TM_INLINE void tm_dot_prod(mtype_t* sptr, mtype_t* kptr,uint32_t size, sumtype_t* result){
    uint32_t cnt;                       
    sumtype_t sum = 0;                                             
    cnt = size/PARALLEL_CNT;

    float32x4_t _sum = vdupq_n_f32(0);
    for(int i = 0; i < cnt; i++) {
        float32x4_t s = vld1q_f32(sptr); 
        float32x4_t k = vld1q_f32(kptr);
        _sum = vmlaq_f32(_sum, s, k);
        sptr += PARALLEL_CNT;
        kptr += PARALLEL_CNT;
    }
    float32_t _sums[PARALLEL_CNT];
    vst1q_f32(_sums, _sum);
    for(int i = 0; i < PARALLEL_CNT; i++) 
        sum += _sums[i];
    
    cnt = size % PARALLEL_CNT;
    while (cnt > 0U){
        sum += (int32_t) ((int16_t) *sptr++ * *kptr++);
        cnt--;
    }

    *result = sum;
    return;
}

TM_INLINE  void tm_dot_prod_pack2(mtype_t* sptr, mtype_t* kptr, uint32_t size, sumtype_t* result)
{ 
    uint32_t cnt;                       
    sumtype_t sum0 = 0;  
    sumtype_t sum1 = 0;           
    mtype_t* kptr0 = kptr;
    mtype_t* kptr1 = kptr+size;                                
    cnt = size/PARALLEL_CNT;

    float32x4_t _sum0 = vdupq_n_f32(0);
    float32x4_t _sum1 = vdupq_n_f32(0);
    for(int i = 0; i < cnt; i++) {
        float32x4_t s = vld1q_f32(sptr); 
        float32x4_t k0 = vld1q_f32(kptr0);
        float32x4_t k1 = vld1q_f32(kptr1);
        _sum0 = vmlaq_f32(_sum0, s, k0);
        _sum1 = vmlaq_f32(_sum1, s, k1);
        sptr += PARALLEL_CNT;
        kptr0 += PARALLEL_CNT;
        kptr1 += PARALLEL_CNT;
    }
    float32_t _sums0[PARALLEL_CNT];
    float32_t _sums1[PARALLEL_CNT];
    vst1q_f32(_sums0, _sum0);
    vst1q_f32(_sums1, _sum1);
    for(int i = 0; i < PARALLEL_CNT; i++) {
        sum0 += _sums0[i];
        sum1 += _sums1[i];
    }
    
    cnt = size % PARALLEL_CNT;
    while (cnt > 0U){
        sum0 +=  (*sptr * *kptr0++);
        sum1 +=  (*sptr++ * *kptr1++);
        cnt--;
    }

    result[0] = sum0;
    result[1] = sum1;
    return;
}

TM_INLINE void tm_dot_prod_gap_3x3x1(mtype_t* sptr, mtype_t* kptr, uint32_t* k_oft, sumtype_t* result)
{
    *result = sptr[k_oft[0]]*kptr[0] + sptr[k_oft[1]]*kptr[1] + sptr[k_oft[2]]*kptr[2] + \
        sptr[k_oft[3]]*kptr[3] + sptr[k_oft[4]]*kptr[4] + sptr[k_oft[5]]*kptr[5] + \
        sptr[k_oft[6]]*kptr[6] + sptr[k_oft[7]]*kptr[7] + sptr[k_oft[8]]*kptr[8] ;
    return;                  
}

TM_INLINE void tm_dot_prod_3x3x1(mtype_t* sptr, mtype_t* kptr, sumtype_t* result)
{
    *result = sptr[0]*kptr[0] + sptr[1]*kptr[1] + sptr[2]*kptr[2] + \
        sptr[3]*kptr[3] + sptr[4]*kptr[4] + sptr[5]*kptr[5] + \
        sptr[6]*kptr[6] + sptr[7]*kptr[7] + sptr[8]*kptr[8] ;
    return;
}

#else  
    #error "TODO"
#endif


#if (TM_MDL_TYPE==TM_MDL_FP32) || (TM_MDL_TYPE==TM_MDL_FP16) 

TM_INLINE void tm_postprocess_sum(int n, sumtype_t* sums, btype_t* bs, int act, mtype_t* outp, \
    sctype_t* scales, sctype_t out_s, zptype_t out_zp)
{
    for(int i = 0; i < n; i++) {
        sumtype_t sum = sums[i];
        sum += bs[i];
        switch(act){    //activation func
        case TM_ACT_RELU:
        case TM_ACT_RELU6: //treat relu6 as relu in float mode //speed up
            sum = sum>0?sum:0;
            break;
        //    sum = sum>0?sum:0;
        //    sum = sum>6?6:sum;
        //    break;
        default:
            break;
        }
        outp[i] = (mtype_t)sum;
    }
    return;
}

#elif (TM_MDL_TYPE==TM_MDL_INT8) || (TM_MDL_TYPE==TM_MDL_INT16) 

#if !TM_FASTSCALE
TM_INLINE void tm_postprocess_sum(int n, sumtype_t* sums, btype_t* bs, int act, mtype_t* outp, sctype_t* scales, sctype_t out_s_inv, zptype_t out_zp)
#else
TM_INLINE void tm_postprocess_sum(int n, sumtype_t* sums, btype_t* bs, int act, mtype_t* outp, int32_t* scales, int32_t out_s, zptype_t out_zp)
#endif
{
    for(int i = 0; i < n; i++) {
        sumtype_t sum = sums[i];
        sum += bs[i];
        #if !TM_FASTSCALE
            float sumf = sum*scales[i];
        #else 
            sumtype_t sumf = (sum<<TM_FASTSCALE_SHIFT)/scales[i];
        #endif
        switch(act){    //activation func
        case TM_ACT_RELU:
            sumf = sumf>0?sumf:0;
            break;
        case TM_ACT_RELU6:
            sumf = sumf>0?sumf:0;
        #if (!TM_FASTSCALE)
            sumf = sumf>6?6:sumf;
        #else
            sumf = sumf>(6<<TM_FASTSCALE_SHIFT)?(6<<TM_FASTSCALE_SHIFT):sumf;
        #endif
            break;
        default:
            break;
        }
        #if !TM_FASTSCALE
            outp[i] = (mtype_t)(sumf*out_s_inv + out_zp);  //(mtype_t)((int)(sumf/out_s) + out_zp) //(mtype_t)((int)(sumf/out_s +0.5) + out_zp)
        #else 
            outp[i] = (mtype_t)(((sumf*out_s)>>(TM_FASTSCALE_SHIFT+TM_FASTSCALE_SHIFT))+out_zp);
        #endif
    }
    return;
}
#endif

