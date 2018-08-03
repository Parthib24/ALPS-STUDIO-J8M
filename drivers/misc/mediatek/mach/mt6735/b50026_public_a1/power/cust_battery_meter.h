#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

//#include <mt_typedefs.h>

#if defined(CONFIG_ARCH_MT6735)
/* ============================================================
// define
// ============================================================*/
/*#define SOC_BY_AUXADC*/
#define SOC_BY_HW_FG
/*#define HW_FG_FORCE_USE_SW_OCV*/
/*#define SOC_BY_SW_FG*/

/*
//#define CONFIG_DIS_CHECK_BATTERY
//#define FIXED_TBAT_25
*/

#if defined(MTK_BQ24196_SUPPORT)\
	||defined(MTK_BQ24296_SUPPORT)\
	||defined(MTK_BQ24160_SUPPORT)\
	||defined(MTK_BQ24261_SUPPORT)
#define SWCHR_POWER_PATH
#endif

#if defined(MTK_FAN5402_SUPPORT) \
	 || defined(MTK_FAN5405_SUPPORT) \
	  || defined(MTK_BQ24158_SUPPORT) \
	   || defined(MTK_BQ24196_SUPPORT) \
	    || defined(MTK_BQ24296_SUPPORT) \
	     || defined(MTK_NCP1851_SUPPORT) \
	      || defined(MTK_NCP1854_SUPPORT) \
	       || defined(MTK_BQ24160_SUPPORT) \
	        || defined(MTK_BQ24157_SUPPORT) \
	         || defined(MTK_BQ24250_SUPPORT) \
	          || defined(MTK_BQ24261_SUPPORT) 
#define EXTERNAL_SWCHR_SUPPORT
#endif

/* ADC resistor  */
#define R_BAT_SENSE 4
#define R_I_SENSE 4
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0 110
#define TEMPERATURE_T1 0
#define TEMPERATURE_T2 25
#define TEMPERATURE_T3 50
#define TEMPERATURE_T 255 /* This should be fixed, never change the value*/

#define FG_METER_RESISTANCE 0

/* Qmax for battery  */
#define Q_MAX_POS_50	2049               
#define Q_MAX_POS_25	2074               
#define Q_MAX_POS_0	1957	               
#define Q_MAX_NEG_10	1879               
                                         
#define Q_MAX_POS_50_H_CURRENT	2030     
#define Q_MAX_POS_25_H_CURRENT	2040     
#define Q_MAX_POS_0_H_CURRENT	  1783     
#define Q_MAX_NEG_10_H_CURRENT	1543     


/* Discharge Percentage */
#define OAM_D5		 1		/*  1 : D5,   0: D2*/


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#ifdef CONFIG_MTK_HAFG_20
#define CUST_TRACKING_POINT  0
#else
#define CUST_TRACKING_POINT  1
#endif
#define CUST_R_SENSE 56
#define CUST_HW_CC 0
#define AGING_TUNING_VALUE 103
#define CUST_R_FG_OFFSET 0

#define OCV_BOARD_COMPESATE	0 /*mV */
#define R_FG_BOARD_BASE 1000
#define R_FG_BOARD_SLOPE 1000 /*slope*/
#define CAR_TUNE_VALUE  79 /*1.00*/


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG 10  /*1mA*/
#define MinErrorOffset 1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE 10 /* mOhm, base is 20*/

/* fg 2.0 */
#define DIFFERENCE_HWOCV_RTC		30
#define DIFFERENCE_HWOCV_SWOCV		10
#define DIFFERENCE_SWOCV_RTC		10
#define MAX_SWOCV			3

#define DIFFERENCE_VOLTAGE_UPDATE	20
#define AGING1_LOAD_SOC			70
#define AGING1_UPDATE_SOC		30
#define BATTERYPSEUDO100		98
#define BATTERYPSEUDO1			4

/* #define Q_MAX_BY_SYS */
#define Q_MAX_SYS_VOLTAGE		3300
#define SHUTDOWN_GAUGE0
#define SHUTDOWN_GAUGE1_XMINS
#define SHUTDOWN_GAUGE1_MINS		60

#define SHUTDOWN_SYSTEM_VOLTAGE		3350
#define CHARGE_TRACKING_TIME		60
#define DISCHARGE_TRACKING_TIME		10

#define RECHARGE_TOLERANCE		10
/* SW Fuel Gauge */
#define MAX_HWOCV			5
#define MAX_VBAT			90
#define DIFFERENCE_HWOCV_VBAT		30

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		2
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	10


/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP		3600
#define VBAT_LOW_POWER_WAKEUP		3500
#define NORMAL_WAKEUP_PERIOD		5400
#define LOW_POWER_WAKEUP_PERIOD		300
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30

#define INIT_SOC_BY_SW_SOC
/*
//#define SYNC_UI_SOC_IMM			//3. UI SOC sync to FG SOC immediately
*/
#define MTK_ENABLE_AGING_ALGORITHM	/*6. Q_MAX aging algorithm*/
#define MD_SLEEP_CURRENT_CHECK	/*5. Gauge Adjust by OCV 9. MD sleep current check*/
/*
//#define Q_MAX_BY_CURRENT		//7. Qmax variant by current loading.
*/
#define FG_BAT_INT
#define IS_BATTERY_REMOVE_BY_PMIC

#elif defined(CONFIG_ARCH_MT6735M)
/* ============================================================
// define
// ============================================================*/
/*#define SOC_BY_AUXADC*/
#define SOC_BY_HW_FG
/*#define HW_FG_FORCE_USE_SW_OCV*/
/*#define SOC_BY_SW_FG*/

/*
//#define CONFIG_DIS_CHECK_BATTERY
//#define FIXED_TBAT_25
*/

#if defined(CONFIG_MTK_BQ24196_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24296_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24261_SUPPORT)
#define SWCHR_POWER_PATH
#define EXTERNAL_SWCHR_SUPPORT
#endif

/* ADC resistor  */
#define R_BAT_SENSE 4
#define R_I_SENSE 4
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0 110
#define TEMPERATURE_T1 0
#define TEMPERATURE_T2 25
#define TEMPERATURE_T3 50
#define TEMPERATURE_T 255 /* This should be fixed, never change the value*/

#define FG_METER_RESISTANCE 0

/* Qmax for battery  */
#define Q_MAX_POS_50	2049  
#define Q_MAX_POS_25	2074
#define Q_MAX_POS_0	1957	
#define Q_MAX_NEG_10	1879

#define Q_MAX_POS_50_H_CURRENT	2030  
#define Q_MAX_POS_25_H_CURRENT	2040
#define Q_MAX_POS_0_H_CURRENT	  1783
#define Q_MAX_NEG_10_H_CURRENT	1543


/* Discharge Percentage */
#define OAM_D5		 1		/*  1 : D5,   0: D2*/


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#ifdef CONFIG_MTK_HAFG_20
#define CUST_TRACKING_POINT  0
#else
#define CUST_TRACKING_POINT  1
#endif
#define CUST_R_SENSE 56
#define CUST_HW_CC 0
#define AGING_TUNING_VALUE 103
#define CUST_R_FG_OFFSET 0

#define OCV_BOARD_COMPESATE	0 /*mV */
#define R_FG_BOARD_BASE 1000
#define R_FG_BOARD_SLOPE 1000 /*slope*/
#define CAR_TUNE_VALUE		79 //1.00


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG 10  /*1mA*/
#define MinErrorOffset 1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE 10 /* mOhm, base is 20*/

/* fg 2.0 */
#define DIFFERENCE_HWOCV_RTC		30
#define DIFFERENCE_HWOCV_SWOCV		10
#define DIFFERENCE_SWOCV_RTC		10
#define MAX_SWOCV			3

#define DIFFERENCE_VOLTAGE_UPDATE	20
#define AGING1_LOAD_SOC			70
#define AGING1_UPDATE_SOC		30
#define BATTERYPSEUDO100		98
#define BATTERYPSEUDO1			4

/* #define Q_MAX_BY_SYS */
#define Q_MAX_SYS_VOLTAGE		3300
#define SHUTDOWN_GAUGE0
#define SHUTDOWN_GAUGE1_XMINS
#define SHUTDOWN_GAUGE1_MINS		60

#define SHUTDOWN_SYSTEM_VOLTAGE		3350
#define CHARGE_TRACKING_TIME		60
#define DISCHARGE_TRACKING_TIME		10

#define RECHARGE_TOLERANCE		10
/* SW Fuel Gauge */
#define MAX_HWOCV			5
#define MAX_VBAT			90
#define DIFFERENCE_HWOCV_VBAT		30

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		2
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	10


/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP		3600
#define VBAT_LOW_POWER_WAKEUP		3500
#define NORMAL_WAKEUP_PERIOD		5400
#define LOW_POWER_WAKEUP_PERIOD		300
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30

#define INIT_SOC_BY_SW_SOC
/*
//#define SYNC_UI_SOC_IMM			//3. UI SOC sync to FG SOC immediately
*/
#define MTK_ENABLE_AGING_ALGORITHM	/*6. Q_MAX aging algorithm*/
#define MD_SLEEP_CURRENT_CHECK	/*5. Gauge Adjust by OCV 9. MD sleep current check*/
/*
//#define Q_MAX_BY_CURRENT		//7. Qmax variant by current loading.
*/
#define FG_BAT_INT
#define IS_BATTERY_REMOVE_BY_PMIC



#elif defined(CONFIG_ARCH_MT6753)
/* ============================================================
// define
// ============================================================*/
/*#define SOC_BY_AUXADC*/
#define SOC_BY_HW_FG
/*#define HW_FG_FORCE_USE_SW_OCV*/
/*#define SOC_BY_SW_FG*/

/*
//#define CONFIG_DIS_CHECK_BATTERY
//#define FIXED_TBAT_25
*/

#if defined(CONFIG_MTK_BQ24196_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24296_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24261_SUPPORT)
#define SWCHR_POWER_PATH
#define EXTERNAL_SWCHR_SUPPORT
#endif

/* ADC resistor  */
#define R_BAT_SENSE 4
#define R_I_SENSE 4
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0 110
#define TEMPERATURE_T1 0
#define TEMPERATURE_T2 25
#define TEMPERATURE_T3 50
#define TEMPERATURE_T 255 /* This should be fixed, never change the value*/

#define FG_METER_RESISTANCE 0

/* Qmax for battery  */
#define Q_MAX_POS_50	2049  
#define Q_MAX_POS_25	2074
#define Q_MAX_POS_0	1957	
#define Q_MAX_NEG_10	1879

#define Q_MAX_POS_50_H_CURRENT	2030  
#define Q_MAX_POS_25_H_CURRENT	2040
#define Q_MAX_POS_0_H_CURRENT	  1783
#define Q_MAX_NEG_10_H_CURRENT	1543


/* Discharge Percentage */
#define OAM_D5		 1		/*  1 : D5,   0: D2*/


/* battery meter parameter */
#define CHANGE_TRACKING_POINT
#ifdef CONFIG_MTK_HAFG_20
#define CUST_TRACKING_POINT  0
#else
#define CUST_TRACKING_POINT  1
#endif
#define CUST_R_SENSE 56
#define CUST_HW_CC 0
#define AGING_TUNING_VALUE 103
#define CUST_R_FG_OFFSET 0

#define OCV_BOARD_COMPESATE	0 /*mV */
#define R_FG_BOARD_BASE 1000
#define R_FG_BOARD_SLOPE 1000 /*slope*/
#define CAR_TUNE_VALUE 79/*1.00*/


/* HW Fuel gague  */
#define CURRENT_DETECT_R_FG 10  /*1mA*/
#define MinErrorOffset 1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE 10 /* mOhm, base is 20*/

/* fg 2.0 */
#define DIFFERENCE_HWOCV_RTC		30
#define DIFFERENCE_HWOCV_SWOCV		10
#define DIFFERENCE_SWOCV_RTC		10
#define MAX_SWOCV			3

#define DIFFERENCE_VOLTAGE_UPDATE	20
#define AGING1_LOAD_SOC			70
#define AGING1_UPDATE_SOC		30
#define BATTERYPSEUDO100		98
#define BATTERYPSEUDO1			4

/* #define Q_MAX_BY_SYS */
#define Q_MAX_SYS_VOLTAGE		3300
#define SHUTDOWN_GAUGE0
#define SHUTDOWN_GAUGE1_XMINS
#define SHUTDOWN_GAUGE1_MINS		60

#define SHUTDOWN_SYSTEM_VOLTAGE		3350
#define CHARGE_TRACKING_TIME		60
#define DISCHARGE_TRACKING_TIME		10

#define RECHARGE_TOLERANCE		10
/* SW Fuel Gauge */
#define MAX_HWOCV			5
#define MAX_VBAT			90
#define DIFFERENCE_HWOCV_VBAT		30

/* fg 1.0 */
#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		2
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30
#define CUST_POWERON_DELTA_HW_SW_OCV_CAPACITY_TOLRANCE	10


/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

/* Dynamic change wake up period of battery thread when suspend*/
#define VBAT_NORMAL_WAKEUP		3600
#define VBAT_LOW_POWER_WAKEUP		3500
#define NORMAL_WAKEUP_PERIOD		5400
#define LOW_POWER_WAKEUP_PERIOD		300
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30

#define INIT_SOC_BY_SW_SOC
/*
//#define SYNC_UI_SOC_IMM			//3. UI SOC sync to FG SOC immediately
*/
#define MTK_ENABLE_AGING_ALGORITHM	/*6. Q_MAX aging algorithm*/
#define MD_SLEEP_CURRENT_CHECK	/*5. Gauge Adjust by OCV 9. MD sleep current check*/
/*
//#define Q_MAX_BY_CURRENT		//7. Qmax variant by current loading.
*/
#define FG_BAT_INT
#define IS_BATTERY_REMOVE_BY_PMIC
/* #define USE_EMBEDDED_BATTERY */

/* Multi battery */
/* #define MTK_MULTI_BAT_PROFILE_SUPPORT */
#endif

#endif	/*#ifndef _CUST_BATTERY_METER_H*/