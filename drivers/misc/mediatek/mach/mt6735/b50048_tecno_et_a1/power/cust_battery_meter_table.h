#ifndef _CUST_BATTERY_METER_TABLE_H
#define _CUST_BATTERY_METER_TABLE_H

#include <mt-plat/battery_meter.h>
#define BAT_NTC_10 1
#define BAT_NTC_47 0

#if (BAT_NTC_10 == 1)
#define RBAT_PULL_UP_R             16900
#endif

#if (BAT_NTC_47 == 1)
#define RBAT_PULL_UP_R             61900
#endif

#define RBAT_PULL_UP_VOLT          1800

typedef struct _BATTERY_PROFILE_STRUCT {
	signed int percentage;
	signed int voltage;
} BATTERY_PROFILE_STRUCT, *BATTERY_PROFILE_STRUCT_P;

typedef struct _R_PROFILE_STRUCT {
	signed int resistance;
	signed int voltage;
} R_PROFILE_STRUCT, *R_PROFILE_STRUCT_P;

typedef enum {
	T1_0C,
	T2_25C,
	T3_50C
} PROFILE_TEMPERATURE;

#if (BAT_NTC_10 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 68237},
	{-15, 53650},
	{-10, 42506},
	{ -5, 33892},
	{  0, 27219},
	{  5, 22021},
	{ 10, 17926},
	{ 15, 14674},
	{ 20, 12081},
	{ 25, 10000},
	{ 30, 8315},
	{ 35, 6948},
	{ 40, 5834},
	{ 45, 4917},
	{ 50, 4161},
	{ 55, 3535},
	{ 60, 3014}
};
#endif

#if (BAT_NTC_47 == 1)
	BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 483954},
	{-15, 360850},
	{-10, 271697},
	{ -5, 206463},
	{  0, 158214},
	{  5, 122259},
	{ 10, 95227},
	{ 15, 74730},
	{ 20, 59065},
	{ 25, 47000},
	{ 30, 37643},
	{ 35, 30334},
	{ 40, 24591},
	{ 45, 20048},
	{ 50, 16433},
	{ 55, 13539},
	{ 60, 11210}
	};
#endif

// T0 -10C
BATTERY_PROFILE_STRUCT battery_profile_t0[] =
{
{0,4337},
{0,4298},
{1,4277},
{2,4259},
{3,4242},
{4,4226},
{5,4211},
{6,4196},
{7,4182},
{8,4168},
{9,4155},
{10,4142},
{11,4129},
{12,4116},
{13,4104},
{14,4091},
{15,4079},
{16,4068},
{17,4058},
{18,4048},
{19,4038},
{20,4026},
{21,4013},
{22,3998},
{23,3981},
{24,3966},
{25,3952},
{26,3939},
{27,3929},
{28,3920},
{29,3911},
{30,3905},
{31,3898},
{32,3892},
{33,3885},
{34,3879},
{35,3872},
{36,3865},
{37,3859},
{38,3852},
{39,3846},
{40,3839},
{41,3833},
{42,3827},
{43,3821},
{44,3816},
{45,3811},
{46,3805},
{47,3800},
{48,3794},
{49,3790},
{50,3785},
{51,3781},
{52,3777},
{53,3773},
{54,3770},
{55,3766},
{56,3763},
{57,3761},
{58,3758},
{59,3756},
{60,3753},
{61,3751},
{62,3748},
{63,3746},
{64,3744},
{65,3741},
{66,3739},
{67,3736},
{68,3734},
{69,3732},
{70,3729},
{71,3727},
{72,3724},
{73,3721},
{74,3718},
{75,3715},
{76,3713},
{77,3710},
{78,3706},
{79,3703},
{80,3699},
{81,3696},
{82,3692},
{83,3687},
{84,3683},
{85,3678},
{86,3672},
{87,3665},
{88,3659},
{89,3651},
{90,3643},
{91,3633},
{92,3621},
{93,3608},
{94,3591},
{95,3572},
{96,3547},
{97,3515},
{98,3472},
{99,3438},
{99,3422},
{99,3407},
{100,3400},
{100,3400}

};      
        
// T1 0C 
BATTERY_PROFILE_STRUCT battery_profile_t1[] =
{
{0,4335},
{0,4305},
{1,4287},
{2,4273},
{3,4259},
{4,4247},
{5,4235},
{6,4223},
{7,4211},
{8,4199},
{9,4188},
{10,4177},
{11,4166},
{12,4155},
{13,4144},
{14,4134},
{15,4122},
{16,4112},
{17,4100},
{18,4090},
{19,4079},
{20,4069},
{21,4061},
{22,4053},
{23,4045},
{24,4034},
{25,4020},
{26,4002},
{27,3985},
{28,3970},
{29,3958},
{30,3949},
{31,3940},
{32,3932},
{33,3928},
{34,3923},
{35,3917},
{36,3910},
{37,3903},
{38,3896},
{39,3890},
{40,3883},
{41,3877},
{42,3870},
{43,3864},
{43,3858},
{44,3852},
{45,3846},
{46,3841},
{47,3835},
{48,3830},
{49,3825},
{50,3820},
{51,3816},
{52,3811},
{53,3807},
{54,3803},
{55,3798},
{56,3794},
{57,3791},
{58,3787},
{59,3784},
{60,3780},
{61,3777},
{62,3774},
{63,3771},
{64,3769},
{65,3767},
{66,3765},
{67,3763},
{68,3761},
{69,3759},
{70,3757},
{71,3755},
{72,3753},
{73,3751},
{74,3749},
{75,3746},
{76,3744},
{77,3741},
{78,3738},
{79,3735},
{80,3732},
{81,3728},
{82,3725},
{83,3720},
{84,3716},
{85,3711},
{86,3707},
{87,3702},
{88,3698},
{89,3694},
{90,3690},
{90,3686},
{91,3680},
{92,3674},
{93,3666},
{94,3654},
{95,3636},
{96,3608},
{97,3567},
{98,3508},
{99,3422},
{100,3400},
{100,3400}

};           

// T2 25C
BATTERY_PROFILE_STRUCT battery_profile_t2[] =
{
{0,4350},
{0,4334},
{1,4316},
{2,4299},
{3,4287},
{4,4274},
{5,4264},
{6,4252},
{7,4240},
{8,4227},
{9,4218},
{10,4206},
{11,4196},
{12,4186},
{13,4174},
{14,4163},
{15,4152},
{16,4142},
{17,4130},
{18,4121},
{19,4111},
{20,4098},
{21,4088},
{22,4079},
{23,4067},
{24,4059},
{25,4055},
{26,4044},
{27,4028},
{28,4013},
{29,3999},
{30,3992},
{31,3983},
{32,3980},
{33,3973},
{34,3966},
{35,3956},
{36,3949},
{37,3942},
{38,3933},
{39,3924},
{40,3912},
{41,3907},
{42,3899},
{43,3888},
{44,3882},
{45,3872},
{46,3867},
{47,3858},
{48,3855},
{49,3849},
{50,3842},
{51,3839},
{52,3834},
{53,3828},
{54,3823},
{55,3821},
{56,3816},
{57,3811},
{58,3808},
{59,3805},
{60,3801},
{61,3796},
{62,3794},
{63,3791},
{64,3787},
{65,3784},
{66,3781},
{67,3779},
{68,3776},
{69,3774},
{70,3771},
{71,3768},
{72,3766},
{73,3763},
{74,3760},
{75,3758},
{76,3755},
{77,3751},
{78,3749},
{79,3743},
{80,3741},
{81,3737},
{82,3734},
{83,3730},
{84,3723},
{85,3720},
{86,3713},
{87,3707},
{88,3701},
{89,3692},
{90,3686},
{91,3684},
{92,3682},
{93,3678},
{94,3671},
{95,3649},
{96,3613},
{97,3590},
{98,3554},
{99,3469},
{100,3400},
{100,3400},
{100,3400},
{100,3400}
 
};     

// T3 50C
BATTERY_PROFILE_STRUCT battery_profile_t3[] =
{
{0,4328},
{0,4313},
{1,4299},
{2,4286},
{3,4274},
{4,4262},
{5,4250},
{6,4239},
{7,4228},
{8,4217},
{9,4206},
{10,4195},
{11,4185},
{12,4174},
{13,4163},
{14,4153},
{15,4142},
{16,4131},
{17,4121},
{18,4111},
{19,4100},
{20,4091},
{21,4081},
{22,4071},
{23,4061},
{24,4051},
{25,4042},
{26,4033},
{27,4024},
{27,4014},
{28,4005},
{29,3997},
{30,3988},
{31,3980},
{32,3971},
{33,3964},
{34,3955},
{35,3948},
{36,3940},
{37,3932},
{38,3924},
{39,3916},
{40,3907},
{41,3898},
{42,3888},
{43,3879},
{44,3870},
{45,3862},
{46,3854},
{47,3847},
{48,3841},
{49,3835},
{50,3829},
{51,3825},
{52,3820},
{53,3815},
{54,3811},
{55,3807},
{55,3803},
{56,3798},
{57,3795},
{58,3791},
{59,3788},
{60,3784},
{61,3781},
{62,3777},
{63,3774},
{64,3772},
{65,3769},
{66,3766},
{67,3763},
{68,3760},
{69,3758},
{70,3754},
{71,3751},
{72,3746},
{73,3741},
{74,3737},
{75,3733},
{76,3730},
{77,3727},
{78,3723},
{79,3719},
{80,3715},
{81,3712},
{82,3709},
{83,3705},
{83,3700},
{84,3694},
{85,3688},
{86,3683},
{87,3676},
{88,3668},
{89,3664},
{90,3663},
{91,3662},
{92,3661},
{93,3659},
{94,3656},
{95,3648},
{96,3625},
{97,3585},
{98,3531},
{99,3460},
{100,3400}


};           

// battery profile for actual temperature. The size should be the same as T1, T2 and T3
BATTERY_PROFILE_STRUCT battery_profile_temperature[] =
{
    {0  , 0 }, 
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },  
	{0  , 0 }, 
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },  
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 }
};    

// ============================================================
// <Rbat, Battery_Voltage> Table
// ============================================================
// T0 -10C
R_PROFILE_STRUCT r_profile_t0[] =
{
{635,4337},
{635,4298},
{648,4277},
{648,4259},
{646,4242},
{641,4226},
{637,4211},
{630,4196},
{625,4182},
{620,4168},
{616,4155},
{611,4142},
{606,4129},
{601,4116},
{597,4104},
{593,4091},
{588,4079},
{585,4068},
{584,4058},
{585,4048},
{587,4038},
{585,4026},
{582,4013},
{575,3998},
{565,3981},
{558,3966},
{550,3952},
{546,3939},
{544,3929},
{544,3920},
{544,3911},
{545,3905},
{546,3898},
{548,3892},
{547,3885},
{548,3879},
{548,3872},
{546,3865},
{548,3859},
{548,3852},
{548,3846},
{548,3839},
{550,3833},
{550,3827},
{551,3821},
{554,3816},
{556,3811},
{557,3805},
{559,3800},
{560,3794},
{562,3790},
{565,3785},
{568,3781},
{571,3777},
{575,3773},
{579,3770},
{581,3766},
{587,3763},
{592,3761},
{596,3758},
{603,3756},
{606,3753},
{613,3751},
{618,3748},
{624,3746},
{633,3744},
{640,3741},
{648,3739},
{657,3736},
{669,3734},
{679,3732},
{691,3729},
{705,3727},
{719,3724},
{734,3721},
{754,3718},
{771,3715},
{793,3713},
{817,3710},
{843,3706},
{870,3703},
{903,3699},
{938,3696},
{979,3692},
{1026,3687},
{1079,3683},
{1133,3678},
{1194,3672},
{1253,3665},
{1307,3659},
{1353,3651},
{1389,3643},
{1423,3633},
{1457,3621},
{1499,3608},
{1554,3591},
{1623,3572},
{1726,3547},
{1879,3515},
{2136,3472},
{2349,3438},
{2316,3422},
{2278,3407},
{2255,3400},
{2220,3400}
 
};      

// T1 0C
R_PROFILE_STRUCT r_profile_t1[] =
{
{250,4335},
{250,4305},
{255,4287},
{258,4273},
{260,4259},
{262,4247},
{262,4235},
{265,4223},
{266,4211},
{267,4199},
{269,4188},
{272,4177},
{273,4166},
{299,4155},
{381,4144},
{382,4134},
{381,4122},
{383,4112},
{380,4100},
{380,4090},
{379,4079},
{379,4069},
{379,4061},
{382,4053},
{383,4045},
{383,4034},
{382,4020},
{378,4002},
{374,3985},
{370,3970},
{370,3958},
{402,3949},
{406,3940},
{412,3932},
{406,3928},
{390,3923},
{401,3917},
{405,3910},
{406,3903},
{407,3896},
{407,3890},
{405,3883},
{404,3877},
{403,3870},
{402,3864},
{402,3858},
{402,3852},
{401,3846},
{401,3841},
{401,3835},
{401,3830},
{401,3825},
{402,3820},
{402,3816},
{401,3811},
{403,3807},
{402,3803},
{403,3798},
{403,3794},
{403,3791},
{404,3787},
{404,3784},
{404,3780},
{405,3777},
{405,3774},
{406,3771},
{406,3769},
{407,3767},
{408,3765},
{408,3763},
{410,3761},
{411,3759},
{410,3757},
{413,3755},
{414,3753},
{416,3751},
{417,3749},
{419,3746},
{421,3744},
{424,3741},
{425,3738},
{427,3735},
{429,3732},
{434,3728},
{436,3725},
{439,3720},
{442,3716},
{447,3711},
{452,3707},
{456,3702},
{464,3698},
{473,3694},
{485,3690},
{500,3686},
{517,3680},
{544,3674},
{577,3666},
{620,3654},
{675,3636},
{751,3608},
{862,3567},
{1042,3508},
{1304,3422},
{1715,3400},
{1842,3400}  

};     

// T2 25C
R_PROFILE_STRUCT r_profile_t2[] =
{
{151,4296},
{151,4281},
{151,4268},
{152,4257},
{151,4245},
{152,4233},
{153,4222},
{152,4211},
{152,4200},
{153,4189},
{153,4178},
{155,4167},
{154,4157},
{155,4146},
{155,4135},
{156,4125},
{158,4115},
{156,4104},
{158,4094},
{158,4083},
{159,4073},
{160,4063},
{160,4054},
{162,4047},
{164,4039},
{164,4028},
{161,4014},
{163,4001},
{162,3989},
{162,3978},
{163,3970},
{163,3963},
{162,3957},
{162,3949},
{161,3941},
{161,3933},
{158,3924},
{158,3915},
{155,3906},
{153,3898},
{151,3890},
{148,3881},
{149,3874},
{146,3866},
{145,3860},
{145,3853},
{144,3847},
{143,3841},
{144,3836},
{143,3830},
{143,3825},
{142,3820},
{143,3816},
{143,3812},
{144,3807},
{144,3803},
{145,3799},
{144,3795},
{143,3792},
{145,3789},
{144,3785},
{145,3782},
{145,3778},
{145,3775},
{145,3772},
{145,3770},
{145,3767},
{146,3764},
{146,3762},
{145,3759},
{145,3757},
{145,3754},
{145,3752},
{146,3750},
{145,3748},
{147,3746},
{145,3743},
{145,3741},
{145,3737},
{145,3735},
{145,3731},
{146,3728},
{146,3724},
{146,3720},
{147,3715},
{148,3709},
{146,3703},
{148,3698},
{149,3691},
{148,3683},
{147,3678},
{148,3676},
{151,3674},
{154,3673},
{157,3670},
{161,3666},
{165,3659},
{167,3640},
{170,3600},
{179,3545},
{197,3469},
{238,3400},
{815,3400},
{714,3400},
{694,3400}    
  
}; 

// T3 50C
R_PROFILE_STRUCT r_profile_t3[] =
{
{116,4328},
{116,4313},
{115,4299},
{114,4286},
{115,4274},
{114,4262},
{113,4250},
{114,4239},
{115,4228},
{114,4217},
{115,4206},
{114,4195},
{117,4185},
{117,4174},
{116,4163},
{117,4153},
{117,4142},
{118,4131},
{118,4121},
{119,4111},
{119,4100},
{120,4091},
{121,4081},
{120,4071},
{122,4061},
{121,4051},
{122,4042},
{122,4033},
{124,4024},
{124,4014},
{124,4005},
{124,3997},
{126,3988},
{125,3980},
{127,3971},
{128,3964},
{128,3955},
{128,3948},
{129,3940},
{130,3932},
{129,3924},
{130,3916},
{128,3907},
{127,3898},
{124,3888},
{123,3879},
{120,3870},
{118,3862},
{117,3854},
{114,3847},
{114,3841},
{114,3835},
{113,3829},
{114,3825},
{114,3820},
{114,3815},
{114,3811},
{115,3807},
{115,3803},
{116,3798},
{116,3795},
{117,3791},
{118,3788},
{118,3784},
{118,3781},
{118,3777},
{118,3774},
{120,3772},
{120,3769},
{120,3766},
{119,3763},
{119,3760},
{119,3758},
{118,3754},
{119,3751},
{117,3746},
{116,3741},
{114,3737},
{116,3733},
{115,3730},
{117,3727},
{116,3723},
{115,3719},
{116,3715},
{116,3712},
{117,3709},
{117,3705},
{117,3700},
{116,3694},
{117,3688},
{117,3683},
{119,3676},
{117,3668},
{114,3664},
{116,3663},
{119,3662},
{121,3661},
{124,3659},
{124,3656},
{124,3648},
{122,3625},
{124,3585},
{126,3531},
{130,3460},
{140,3400}
 

}; 

// r-table profile for actual temperature. The size should be the same as T1, T2 and T3
R_PROFILE_STRUCT r_profile_temperature[] =
{
  {0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },  
	{0  , 0 }, 
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },  
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 },
	{0  , 0 }

};    

/* ============================================================
// function prototype
// ============================================================*/
int fgauge_get_saddles(void);
BATTERY_PROFILE_STRUCT_P fgauge_get_profile(unsigned int temperature);

int fgauge_get_saddles_r_table(void);
R_PROFILE_STRUCT_P fgauge_get_profile_r_table(unsigned int temperature);

#endif	/*#ifndef _CUST_BATTERY_METER_TABLE_H*/
