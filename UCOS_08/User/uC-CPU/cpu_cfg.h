#ifndef CPU_CFG_MODULE_PRESENT
#define CPU_CFG_MODULE_PRESENT

#define CPU_CFG_TS_32_EN		DEF_ENABLED
#define CPU_CFG_TS_64_EN		DEF_DISABLED

#define CPU_CFG_TS_TMR_SIZE		CPU_WORD_SIZE_32

/***************************************************************/
#if 1                    
#define  CPU_CFG_INT_DIS_MEAS_EN                                             
#endif
                                                              
#define  CPU_CFG_INT_DIS_MEAS_OVRHD_NBR    	1u

#endif /* CPU_CFG_MODULE_PRESENT */
