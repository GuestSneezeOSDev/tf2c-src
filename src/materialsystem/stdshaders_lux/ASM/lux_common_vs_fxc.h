//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	24.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Declarations for Constants and Functions etc
//							For Vertex Shaders only.
//
//===========================================================================//

#ifndef LUX_COMMON_VS_FXC_H_
#define LUX_COMMON_VS_FXC_H_

#include "common_pragmas.h"
#include "common_hlsl_cpp_consts.h"

//#define float2 float
//#define float3 float
//#define float4 float

//===========================================================================//
//	Declaring #define's and custom errors/error ignores ( pragma )
//===========================================================================//

#define COMPILE_ERROR ( 1/0; )

#pragma def ( vs, c0, 0.0f, 1.0f, 2.0f, 0.5f )

//===========================================================================//
//	Declaring constants that won't change.
//===========================================================================//

struct LightInfo
{
	float4 color;						// {xyz} is color	w is light type code (see comment below)
	float4 dir;							// {xyz} is dir		w is light type code
	float4 pos;
	float4 spotParams;
	float4 atten;
};

//===========================================================================//
//	Declaring BOOLEAN VertexShader Constant Registers ( VSREG's )
//	There can be a maximum of 16 according to Microsoft Documentation
//	Ranging from 0-15 After this follows i0 - i15, c0 - c1536+ then s0 - s3
//===========================================================================//

// The g_bLightEnabled registers and g_nLightCountRegister hold the same information regarding
// enabling lights, but callers internal to this file tend to use the loops, while external
// callers will end up using the booleans
const bool			g_bLightEnabled[4]				: register(b0); // b0 - b3
//const bool		g_bLightEnabled[1]				: register(b1);
//const bool		g_bLightEnabled[2]				: register(b2);
//const bool		g_bLightEnabled[3]				: register(b3);
#define				SHADER_SPECIFIC_BOOL_CONST_0			   b4
#define				SHADER_SPECIFIC_BOOL_CONST_1			   b5
#define				SHADER_SPECIFIC_BOOL_CONST_2			   b6
#define				SHADER_SPECIFIC_BOOL_CONST_3			   b7
#define				SHADER_SPECIFIC_BOOL_CONST_4			   b8
#define				SHADER_SPECIFIC_BOOL_CONST_5			   b9
#define				SHADER_SPECIFIC_BOOL_CONST_6			   b10
#define				SHADER_SPECIFIC_BOOL_CONST_7			   b11
#define				SHADER_SPECIFIC_BOOL_CONST_8			   b12 // This and below are new.
#define				SHADER_SPECIFIC_BOOL_CONST_9			   b13
#define				SHADER_SPECIFIC_BOOL_CONST_10			   b14
#define				SHADER_SPECIFIC_BOOL_CONST_11			   b15


//===========================================================================//
//	Declaring INTEGER VertexShader Constant Registers ( VSREG's )
//	There can be a maximum of 16 according to Microsoft Documentation
//	Ranging from 0-15 After this follows c0 - c1536+ then s0 - s3
//===========================================================================//

const int			g_nLightCountRegister			: register(i0);
#define				g_nLightCount					g_nLightCountRegister.x

//===========================================================================//
//	Declaring FLOAT VertexShader Constant Registers ( VSREG's )
//	There can be a maximum of AT LEAST 256 according to Microsoft Documentation
//	Ranging from 0-255+ After this follows b0-b15 and i0 - i15 then s0 - s3
//	The max amount depends on some value specified elsewhere.
//	We can tell they enabled at least 1536! ( See cFlexWeights )
//===========================================================================//

// This register is probably non-functional for whatever reason.
// There is a pragma for it and it goes unused unlike b0, i0 or even s0.
// Might have been a very specific reason why this one is not used. I don't know and we don't need it.
//const float4		???????????						: register(c0);
const float4		cConstants1						: register(c1);
const float4		cEyePosWaterZ					: register(c2);
const float4		cFlexScale						: register(c3); // Only cFlexScale.x used. Binary value for toggling addition of the flex delta stream.
const float4x4		cModelViewProj					: register(c4);
//					cModelViewProj[1]				: register(c5);
//					cModelViewProj[2]				: register(c6);
//					cModelViewProj[3]				: register(c7);
const float4x4		cViewProj						: register(c8);
//					cViewProj[1]					: register(c9);
//					cViewProj[2]					: register(c10); // 10
//					cViewProj[3]					: register(c11);
const float4		cModelViewProjZ					: register(c12); // Used to compute projPosZ without skinning. Using cModelViewProj with FastClip generates incorrect results
const float4		cViewProjZ						: register(c13); // "More constants working back from the top..."
#define				SHADER_SPECIFIC_CONST_10				   c14
#define				SHADER_SPECIFIC_CONST_11				   c15
const float4		cFogParams						: register(c16);
const float4x4		cViewModel						: register(c17);
//					cViewModel[1]					: register(c18);
//					cViewModel[2]					: register(c19);
//					cViewModel[3]					: register(c20); // 20
const float3		cAmbientCubeX[2]				: register(c21);
//					cAmbientCubeX[1]				: register(c22);
const float3		cAmbientCubeY[2]				: register(c23);
//					cAmbientCubeY[1]				: register(c24);
const float3		cAmbientCubeZ[2]				: register(c25);
//					cAmbientCubeZ[1]				: register(c26);
LightInfo			cLightInfo[4]					: register(c27);
//					cLightInfo[0].dir				: register(c28);
//					cLightInfo[0].pos				: register(c29);
//					cLightInfo[0].spotParams		: register(c30); // 30
//					cLightInfo[0].atten				: register(c31);
//					cLightInfo[1].color				: register(c32);
//					cLightInfo[1].dir				: register(c33);
//					cLightInfo[1].pos				: register(c34);
//					cLightInfo[1].spotParams		: register(c35);
//					cLightInfo[1].atten				: register(c36);
//					cLightInfo[2].color				: register(c37);
//					cLightInfo[2].dir				: register(c38);
//					cLightInfo[2].pos				: register(c39);
//					cLightInfo[2].spotParams		: register(c40); // 40
//					cLightInfo[2].atten				: register(c41);
//					cLightInfo[3].color				: register(c42);
//					cLightInfo[3].dir				: register(c43);
//					cLightInfo[3].pos				: register(c44);
//					cLightInfo[3].spotParams		: register(c45);
//					cLightInfo[3].atten				: register(c46);
const float4		cModulationColor				: register(c47);
#define				SHADER_SPECIFIC_CONST_0					   c48
#define				SHADER_SPECIFIC_CONST_1					   c49
#define				SHADER_SPECIFIC_CONST_2					   c50	 // 50
#define				SHADER_SPECIFIC_CONST_3					   c51
#define				SHADER_SPECIFIC_CONST_4					   c52
#define				SHADER_SPECIFIC_CONST_5					   c53
#define				SHADER_SPECIFIC_CONST_6					   c54
#define				SHADER_SPECIFIC_CONST_7					   c55
#define				SHADER_SPECIFIC_CONST_8					   c56
#define				SHADER_SPECIFIC_CONST_9					   c57
// Handy #ifndef... Skip scrolling 159 instances of		cModel[c58-c222]
#ifndef BLABLABLA
const float4x3		cModel[53]						: register(c58);
//					cModel[00][1]					: register(c59);
//					cModel[00][2]					: register(c60); // 60
//					cModel[01][0]					: register(c61);
//					cModel[01][1]					: register(c62);
//					cModel[01][2]					: register(c63);
//					cModel[02][0]					: register(c64);
//					cModel[02][1]					: register(c65);
//					cModel[02][2]					: register(c66);
//					cModel[03][0]					: register(c67);
//					cModel[03][1]					: register(c68);
//					cModel[03][2]					: register(c69);
//					cModel[04][0]					: register(c70); // 70
//					cModel[04][1]					: register(c71);
//					cModel[04][2]					: register(c72);
//					cModel[05][0]					: register(c73);
//					cModel[05][1]					: register(c74);
//					cModel[05][2]					: register(c75);
//					cModel[06][0]					: register(c76);
//					cModel[06][1]					: register(c77);
//					cModel[06][2]					: register(c78);
//					cModel[07][0]					: register(c79);
//					cModel[07][1]					: register(c80); // 80
//					cModel[07][2]					: register(c81);
//					cModel[08][0]					: register(c82);
//					cModel[08][1]					: register(c83);
//					cModel[08][2]					: register(c84);
//					cModel[09][0]					: register(c85);
//					cModel[09][1]					: register(c86);
//					cModel[09][2]					: register(c87);
//					cModel[10][0]					: register(c88);
//					cModel[10][1]					: register(c89);
//					cModel[10][2]					: register(c90); // 90
//					cModel[11][0]					: register(c91);
//					cModel[11][1]					: register(c92);
//					cModel[11][2]					: register(c93);
//					cModel[12][0]					: register(c94);
//					cModel[12][1]					: register(c95);
//					cModel[12][2]					: register(c96);
//					cModel[13][0]					: register(c97);
//					cModel[13][1]					: register(c98);
//					cModel[13][2]					: register(c99);
//					cModel[14][0]					: register(c100);// 100
//					cModel[14][1]					: register(c101);
//					cModel[14][2]					: register(c102);
//					cModel[15][0]					: register(c103);
//					cModel[15][1]					: register(c104);
//					cModel[15][2]					: register(c105); // last cmodel on dx80
//					cModel[16][0]					: register(c106);
//					cModel[16][1]					: register(c107);
//					cModel[16][2]					: register(c108);
//					cModel[17][0]					: register(c109);
//					cModel[17][1]					: register(c110);// 110
//					cModel[17][2]					: register(c111);
//					cModel[18][0]					: register(c112);
//					cModel[18][1]					: register(c113);
//					cModel[18][2]					: register(c114);
//					cModel[19][0]					: register(c115);
//					cModel[19][1]					: register(c116);
//					cModel[19][2]					: register(c117);
//					cModel[20][0]					: register(c118);
//					cModel[20][1]					: register(c119);
//					cModel[20][2]					: register(c120);// 120
//					cModel[21][0]					: register(c121);
//					cModel[21][1]					: register(c122);
//					cModel[21][2]					: register(c123);
//					cModel[22][0]					: register(c124);
//					cModel[22][1]					: register(c125);
//					cModel[22][2]					: register(c126);
//					cModel[23][0]					: register(c127);
//					cModel[23][1]					: register(c128);
//					cModel[23][2]					: register(c129);
//					cModel[24][0]					: register(c130);// 130
//					cModel[24][1]					: register(c131);
//					cModel[24][2]					: register(c132);
//					cModel[25][0]					: register(c133);
//					cModel[25][1]					: register(c134);
//					cModel[25][2]					: register(c135);
//					cModel[26][0]					: register(c136);
//					cModel[26][1]					: register(c137);
//					cModel[26][2]					: register(c138);
//					cModel[27][0]					: register(c139);
//					cModel[27][1]					: register(c140);// 140
//					cModel[27][2]					: register(c141);
//					cModel[28][0]					: register(c142);
//					cModel[28][1]					: register(c143);
//					cModel[28][2]					: register(c144);
//					cModel[29][0]					: register(c145);
//					cModel[29][1]					: register(c146);
//					cModel[29][2]					: register(c147);
//					cModel[30][0]					: register(c148);
//					cModel[30][1]					: register(c149);
//					cModel[30][2]					: register(c150);// 150
//					cModel[31][0]					: register(c151);
//					cModel[31][1]					: register(c152);
//					cModel[31][2]					: register(c153);
//					cModel[32][0]					: register(c154);
//					cModel[32][1]					: register(c155);
//					cModel[32][2]					: register(c156);
//					cModel[33][0]					: register(c157);
//					cModel[33][1]					: register(c158);
//					cModel[33][2]					: register(c159);
//					cModel[34][0]					: register(c160);// 160
//					cModel[34][1]					: register(c161);
//					cModel[34][2]					: register(c162);
//					cModel[35][0]					: register(c163);
//					cModel[35][1]					: register(c164);
//					cModel[35][2]					: register(c165);
//					cModel[36][0]					: register(c166);
//					cModel[36][1]					: register(c167);
//					cModel[36][2]					: register(c168);
//					cModel[37][0]					: register(c169);
//					cModel[37][1]					: register(c170);// 170
//					cModel[37][2]					: register(c171);
//					cModel[38][0]					: register(c172);
//					cModel[38][1]					: register(c173);
//					cModel[38][2]					: register(c174);
//					cModel[39][0]					: register(c175);
//					cModel[39][1]					: register(c176);
//					cModel[39][2]					: register(c177);
//					cModel[40][0]					: register(c178);
//					cModel[40][1]					: register(c179);
//					cModel[40][2]					: register(c180);// 180
//					cModel[41][0]					: register(c181);
//					cModel[41][1]					: register(c182);
//					cModel[41][2]					: register(c183);
//					cModel[42][0]					: register(c184);
//					cModel[42][1]					: register(c185);
//					cModel[42][2]					: register(c186);
//					cModel[43][0]					: register(c187);
//					cModel[43][1]					: register(c188);
//					cModel[43][2]					: register(c189);
//					cModel[44][0]					: register(c190);// 190
//					cModel[44][1]					: register(c191);
//					cModel[44][2]					: register(c192);
//					cModel[45][0]					: register(c193);
//					cModel[45][1]					: register(c194);
//					cModel[45][2]					: register(c195);
//					cModel[46][0]					: register(c196);
//					cModel[46][1]					: register(c197);
//					cModel[46][2]					: register(c198);
//					cModel[47][0]					: register(c199);
//					cModel[47][1]					: register(c200);// 200
//					cModel[47][2]					: register(c201);
//					cModel[48][0]					: register(c202);
//					cModel[48][1]					: register(c203);
//					cModel[58][2]					: register(c204);
//					cModel[49][0]					: register(c205);
//					cModel[49][1]					: register(c206);
//					cModel[49][2]					: register(c207);
//					cModel[50][0]					: register(c208);
//					cModel[50][1]					: register(c209);
//					cModel[50][2]					: register(c210);// 210
//					cModel[51][0]					: register(c211);
//					cModel[51][1]					: register(c212);
//					cModel[51][2]					: register(c213);
//					cModel[52][0]					: register(c211);
//					cModel[52][1]					: register(c212);
//					cModel[52][2]					: register(c213); // TODO: Could raising the amount of cModel[X] to a higher number ( above 53 ) allow for more bones?
//						EMPTY						: register(c214);
//						EMPTY						: register(c215);
//						EMPTY						: register(c216);
//						EMPTY						: register(c217);
//						EMPTY						: register(c218);
//						EMPTY						: register(c219);
//						EMPTY						: register(c220);// 220
//						EMPTY						: register(c221);
//						EMPTY						: register(c222); // ShiroDkxtro2: From what I can gather, these registers go unused.
#endif
#define				SHADER_SPECIFIC_CONST_13				   c223 
#define				SHADER_SPECIFIC_CONST_14				   c224
#define				SHADER_SPECIFIC_CONST_15				   c225
#define				SHADER_SPECIFIC_CONST_16				   c226
#define				SHADER_SPECIFIC_CONST_17				   c227
#define				SHADER_SPECIFIC_CONST_18				   c228
#define				SHADER_SPECIFIC_CONST_19				   c229
#define				SHADER_SPECIFIC_CONST_20				   c230  // 230
#define				SHADER_SPECIFIC_CONST_21				   c231
#define				SHADER_SPECIFIC_CONST_22				   c232
#define				SHADER_SPECIFIC_CONST_23				   c233
#define				SHADER_SPECIFIC_CONST_24				   c234
#define				SHADER_SPECIFIC_CONST_25				   c235
#define				SHADER_SPECIFIC_CONST_26				   c236
#define				SHADER_SPECIFIC_CONST_27				   c237
#define				SHADER_SPECIFIC_CONST_28				   c238
#define				SHADER_SPECIFIC_CONST_29				   c239
#define				SHADER_SPECIFIC_CONST_30				   c240  // 240
//					ABOUT 800 EMPTY REGISTER		: register(c241);
//								.					: register(c24.);
//							   ...					: register(c2..);
//							  .....					: register(c...);   // These should all be usable.
//							 .......				: register(c...);
//						    .........				: register(c1000............);
// Handy #if... Skip scrolling 512 instances of	  cFlexWeights[c1024 - c1534]
#if defined(MODEL)
const float4		cFlexWeights[512]				: register(c1024);
//					cFlexWeights[001]				: register(c1025);
//					cFlexWeights[002]				: register(c1026);
//					cFlexWeights[003]				: register(c1027);
//					cFlexWeights[004]				: register(c1028);
//					cFlexWeights[005]				: register(c1029);
//					cFlexWeights[006]				: register(c1030);
//					cFlexWeights[007]				: register(c1031);
//					cFlexWeights[008]				: register(c1032);
//					cFlexWeights[009]				: register(c1033);
//					cFlexWeights[011]				: register(c1034);
//					cFlexWeights[012]				: register(c1035);
//					cFlexWeights[013]				: register(c1036);
//					cFlexWeights[014]				: register(c1037);
//					cFlexWeights[015]				: register(c1038);
//					cFlexWeights[016]				: register(c1039);
//					cFlexWeights[017]				: register(c1040);
//					cFlexWeights[018]				: register(c1041);
//					cFlexWeights[019]				: register(c1042);
//					cFlexWeights[020]				: register(c1043);
//					cFlexWeights[021]				: register(c1044);
//					cFlexWeights[022]				: register(c1045);
//					cFlexWeights[023]				: register(c1046);
//					cFlexWeights[024]				: register(c1047);
//					cFlexWeights[025]				: register(c1048);
//					cFlexWeights[026]				: register(c1049);
//					cFlexWeights[027]				: register(c1050);
//					cFlexWeights[028]				: register(c1051);
//					cFlexWeights[029]				: register(c1052);
//					cFlexWeights[030]				: register(c1053);
//					cFlexWeights[031]				: register(c1054);
//					cFlexWeights[032]				: register(c1055);
//					cFlexWeights[033]				: register(c1056);
//					cFlexWeights[034]				: register(c1057);
//					cFlexWeights[035]				: register(c1058);
//					cFlexWeights[036]				: register(c1059);
//					cFlexWeights[037]				: register(c1060);
//					cFlexWeights[038]				: register(c1061);
//					cFlexWeights[039]				: register(c1062);
//					cFlexWeights[040]				: register(c1063);
//					cFlexWeights[041]				: register(c1064);
//					cFlexWeights[042]				: register(c1065);
//					cFlexWeights[043]				: register(c1066);
//					cFlexWeights[044]				: register(c1067);
//					cFlexWeights[045]				: register(c1068);
//					cFlexWeights[046]				: register(c1069);
//					cFlexWeights[047]				: register(c1070);
//					cFlexWeights[048]				: register(c1071);
//					cFlexWeights[049]				: register(c1072);
//					cFlexWeights[050]				: register(c1073);
//					cFlexWeights[051]				: register(c1074);
//					cFlexWeights[052]				: register(c1075);
//					cFlexWeights[053]				: register(c1076);
//					cFlexWeights[054]				: register(c1077);
//					cFlexWeights[055]				: register(c1078);
//					cFlexWeights[056]				: register(c1079);
//					cFlexWeights[057]				: register(c1080);
//					cFlexWeights[058]				: register(c1081);
//					cFlexWeights[059]				: register(c1082);
//					cFlexWeights[060]				: register(c1083);
//					cFlexWeights[061]				: register(c1084);
//					cFlexWeights[062]				: register(c1085);
//					cFlexWeights[063]				: register(c1086);
//					cFlexWeights[064]				: register(c1087);
//					cFlexWeights[065]				: register(c1088);
//					cFlexWeights[066]				: register(c1089);
//					cFlexWeights[067]				: register(c1090);
//					cFlexWeights[068]				: register(c1091);
//					cFlexWeights[069]				: register(c1092);
//					cFlexWeights[070]				: register(c1093);
//					cFlexWeights[071]				: register(c1094);
//					cFlexWeights[072]				: register(c1095);
//					cFlexWeights[073]				: register(c1096);
//					cFlexWeights[074]				: register(c1097);
//					cFlexWeights[075]				: register(c1098);
//					cFlexWeights[076]				: register(c1099);
//					cFlexWeights[077]				: register(c1100);
//					cFlexWeights[078]				: register(c1101);
//					cFlexWeights[079]				: register(c1102);
//					cFlexWeights[080]				: register(c1103);
//					cFlexWeights[081]				: register(c1104);
//					cFlexWeights[082]				: register(c1105);
//					cFlexWeights[083]				: register(c1106);
//					cFlexWeights[084]				: register(c1107);
//					cFlexWeights[085]				: register(c1108);
//					cFlexWeights[086]				: register(c1109);
//					cFlexWeights[087]				: register(c1110);
//					cFlexWeights[088]				: register(c1111);
//					cFlexWeights[089]				: register(c1112);
//					cFlexWeights[090]				: register(c1113);
//					cFlexWeights[091]				: register(c1114);
//					cFlexWeights[092]				: register(c1115);
//					cFlexWeights[093]				: register(c1116);
//					cFlexWeights[094]				: register(c1117);
//					cFlexWeights[095]				: register(c1118);
//					cFlexWeights[096]				: register(c1119);
//					cFlexWeights[097]				: register(c1120);
//					cFlexWeights[098]				: register(c1121);
//					cFlexWeights[099]				: register(c1122);
//					cFlexWeights[100]				: register(c1123);
//					cFlexWeights[101]				: register(c1124);
//					cFlexWeights[102]				: register(c1125);
//					cFlexWeights[103]				: register(c1126);
//					cFlexWeights[104]				: register(c1127);
//					cFlexWeights[105]				: register(c1128);
//					cFlexWeights[106]				: register(c1129);
//					cFlexWeights[107]				: register(c1130);
//					cFlexWeights[108]				: register(c1131);
//					cFlexWeights[109]				: register(c1132);
//					cFlexWeights[110]				: register(c1133);
//					cFlexWeights[111]				: register(c1134);
//					cFlexWeights[112]				: register(c1135);
//					cFlexWeights[113]				: register(c1136);
//					cFlexWeights[114]				: register(c1137);
//					cFlexWeights[115]				: register(c1138);
//					cFlexWeights[116]				: register(c1139);
//					cFlexWeights[117]				: register(c1140);
//					cFlexWeights[118]				: register(c1141);
//					cFlexWeights[119]				: register(c1142);
//					cFlexWeights[120]				: register(c1143);
//					cFlexWeights[121]				: register(c1144);
//					cFlexWeights[122]				: register(c1145);
//					cFlexWeights[123]				: register(c1146);
//					cFlexWeights[124]				: register(c1147);
//					cFlexWeights[125]				: register(c1148);
//					cFlexWeights[126]				: register(c1149);
//					cFlexWeights[127]				: register(c1150);
//					cFlexWeights[128]				: register(c1151);
//					cFlexWeights[129]				: register(c1152);
//					cFlexWeights[130]				: register(c1153);
//					cFlexWeights[131]				: register(c1154);
//					cFlexWeights[132]				: register(c1155);
//					cFlexWeights[133]				: register(c1156);
//					cFlexWeights[134]				: register(c1157);
//					cFlexWeights[135]				: register(c1158);
//					cFlexWeights[136]				: register(c1159);
//					cFlexWeights[137]				: register(c1160);
//					cFlexWeights[138]				: register(c1161);
//					cFlexWeights[139]				: register(c1162);
//					cFlexWeights[140]				: register(c1163);
//					cFlexWeights[141]				: register(c1164);
//					cFlexWeights[142]				: register(c1165);
//					cFlexWeights[143]				: register(c1166);
//					cFlexWeights[144]				: register(c1167);
//					cFlexWeights[145]				: register(c1168);
//					cFlexWeights[146]				: register(c1169);
//					cFlexWeights[147]				: register(c1170);
//					cFlexWeights[148]				: register(c1171);
//					cFlexWeights[149]				: register(c1172);
//					cFlexWeights[150]				: register(c1173);
//					cFlexWeights[151]				: register(c1174);
//					cFlexWeights[152]				: register(c1175);
//					cFlexWeights[153]				: register(c1176);
//					cFlexWeights[154]				: register(c1177);
//					cFlexWeights[155]				: register(c1178);
//					cFlexWeights[156]				: register(c1179);
//					cFlexWeights[157]				: register(c1180);
//					cFlexWeights[158]				: register(c1181);
//					cFlexWeights[159]				: register(c1182);
//					cFlexWeights[160]				: register(c1183);
//					cFlexWeights[161]				: register(c1184);
//					cFlexWeights[162]				: register(c1185);
//					cFlexWeights[163]				: register(c1186);
//					cFlexWeights[164]				: register(c1187);
//					cFlexWeights[165]				: register(c1188);
//					cFlexWeights[166]				: register(c1189);
//					cFlexWeights[167]				: register(c1190);
//					cFlexWeights[168]				: register(c1191);
//					cFlexWeights[169]				: register(c1192);
//					cFlexWeights[170]				: register(c1193);
//					cFlexWeights[171]				: register(c1194);
//					cFlexWeights[172]				: register(c1195);
//					cFlexWeights[173]				: register(c1196);
//					cFlexWeights[174]				: register(c1197);
//					cFlexWeights[175]				: register(c1198);
//					cFlexWeights[176]				: register(c1199);
//					cFlexWeights[177]				: register(c1200);
//					cFlexWeights[178]				: register(c1201);
//					cFlexWeights[179]				: register(c1202);
//					cFlexWeights[180]				: register(c1203);
//					cFlexWeights[181]				: register(c1204);
//					cFlexWeights[182]				: register(c1205);
//					cFlexWeights[183]				: register(c1206);
//					cFlexWeights[184]				: register(c1207);
//					cFlexWeights[185]				: register(c1208);
//					cFlexWeights[186]				: register(c1209);
//					cFlexWeights[187]				: register(c1210);
//					cFlexWeights[188]				: register(c1211);
//					cFlexWeights[189]				: register(c1212);
//					cFlexWeights[190]				: register(c1213);
//					cFlexWeights[191]				: register(c1214);
//					cFlexWeights[192]				: register(c1215);
//					cFlexWeights[193]				: register(c1216);
//					cFlexWeights[194]				: register(c1217);
//					cFlexWeights[195]				: register(c1218);
//					cFlexWeights[196]				: register(c1219);
//					cFlexWeights[197]				: register(c1220);
//					cFlexWeights[198]				: register(c1221);
//					cFlexWeights[199]				: register(c1222);
//					cFlexWeights[200]				: register(c1223);
//					cFlexWeights[201]				: register(c1224);
//					cFlexWeights[202]				: register(c1225);
//					cFlexWeights[203]				: register(c1226);
//					cFlexWeights[204]				: register(c1227);
//					cFlexWeights[205]				: register(c1228);
//					cFlexWeights[206]				: register(c1229);
//					cFlexWeights[207]				: register(c1230);
//					cFlexWeights[208]				: register(c1231);
//					cFlexWeights[209]				: register(c1232);
//					cFlexWeights[210]				: register(c1233);
//					cFlexWeights[211]				: register(c1234);
//					cFlexWeights[212]				: register(c1235);
//					cFlexWeights[213]				: register(c1236);
//					cFlexWeights[214]				: register(c1237);
//					cFlexWeights[215]				: register(c1238);
//					cFlexWeights[216]				: register(c1239);
//					cFlexWeights[217]				: register(c1240);
//					cFlexWeights[218]				: register(c1241);
//					cFlexWeights[219]				: register(c1242);
//					cFlexWeights[220]				: register(c1243);
//					cFlexWeights[221]				: register(c1244);
//					cFlexWeights[222]				: register(c1245);
//					cFlexWeights[223]				: register(c1246);
//					cFlexWeights[224]				: register(c1247);
//					cFlexWeights[225]				: register(c1248);
//					cFlexWeights[226]				: register(c1249);
//					cFlexWeights[227]				: register(c1250);
//					cFlexWeights[228]				: register(c1251);
//					cFlexWeights[229]				: register(c1252);
//					cFlexWeights[230]				: register(c1253);
//					cFlexWeights[231]				: register(c1254);
//					cFlexWeights[232]				: register(c1255);
//					cFlexWeights[233]				: register(c1256);
//					cFlexWeights[234]				: register(c1257);
//					cFlexWeights[235]				: register(c1258);
//					cFlexWeights[236]				: register(c1259);
//					cFlexWeights[237]				: register(c1260);
//					cFlexWeights[238]				: register(c1261);
//					cFlexWeights[239]				: register(c1262);
//					cFlexWeights[240]				: register(c1263);
//					cFlexWeights[241]				: register(c1264);
//					cFlexWeights[242]				: register(c1265);
//					cFlexWeights[243]				: register(c1266);
//					cFlexWeights[244]				: register(c1267);
//					cFlexWeights[245]				: register(c1268);
//					cFlexWeights[246]				: register(c1269);
//					cFlexWeights[247]				: register(c1270);
//					cFlexWeights[248]				: register(c1271);
//					cFlexWeights[249]				: register(c1272);
//					cFlexWeights[250]				: register(c1273);
//					cFlexWeights[251]				: register(c1274);
//					cFlexWeights[252]				: register(c1275);
//					cFlexWeights[253]				: register(c1276);
//					cFlexWeights[254]				: register(c1277);
//					cFlexWeights[255]				: register(c1278);
//					cFlexWeights[256]				: register(c1279);
//					cFlexWeights[257]				: register(c1280);
//					cFlexWeights[258]				: register(c1281);
//					cFlexWeights[259]				: register(c1282);
//					cFlexWeights[260]				: register(c1283);
//					cFlexWeights[261]				: register(c1284);
//					cFlexWeights[262]				: register(c1285);
//					cFlexWeights[263]				: register(c1286);
//					cFlexWeights[264]				: register(c1287);
//					cFlexWeights[265]				: register(c1288);
//					cFlexWeights[266]				: register(c1289);
//					cFlexWeights[267]				: register(c1290);
//					cFlexWeights[268]				: register(c1291);
//					cFlexWeights[269]				: register(c1292);
//					cFlexWeights[270]				: register(c1293);
//					cFlexWeights[271]				: register(c1294);
//					cFlexWeights[272]				: register(c1295);
//					cFlexWeights[273]				: register(c1296);
//					cFlexWeights[274]				: register(c1297);
//					cFlexWeights[275]				: register(c1298);
//					cFlexWeights[276]				: register(c1299);
//					cFlexWeights[277]				: register(c1300);
//					cFlexWeights[278]				: register(c1301);
//					cFlexWeights[279]				: register(c1302);
//					cFlexWeights[280]				: register(c1303);
//					cFlexWeights[281]				: register(c1304);
//					cFlexWeights[282]				: register(c1305);
//					cFlexWeights[283]				: register(c1306);
//					cFlexWeights[284]				: register(c1307);
//					cFlexWeights[285]				: register(c1308);
//					cFlexWeights[286]				: register(c1309);
//					cFlexWeights[287]				: register(c1310);
//					cFlexWeights[288]				: register(c1311);
//					cFlexWeights[289]				: register(c1312);
//					cFlexWeights[290]				: register(c1313);
//					cFlexWeights[291]				: register(c1314);
//					cFlexWeights[292]				: register(c1315);
//					cFlexWeights[293]				: register(c1316);
//					cFlexWeights[294]				: register(c1317);
//					cFlexWeights[295]				: register(c1318);
//					cFlexWeights[296]				: register(c1319);
//					cFlexWeights[297]				: register(c1320);
//					cFlexWeights[298]				: register(c1321);
//					cFlexWeights[299]				: register(c1322);
//					cFlexWeights[300]				: register(c1323);
//					cFlexWeights[301]				: register(c1324);
//					cFlexWeights[302]				: register(c1325);
//					cFlexWeights[303]				: register(c1326);
//					cFlexWeights[304]				: register(c1327);
//					cFlexWeights[305]				: register(c1328);
//					cFlexWeights[306]				: register(c1329);
//					cFlexWeights[307]				: register(c1330);
//					cFlexWeights[308]				: register(c1331);
//					cFlexWeights[309]				: register(c1332);
//					cFlexWeights[310]				: register(c1333);
//					cFlexWeights[311]				: register(c1334);
//					cFlexWeights[312]				: register(c1335);
//					cFlexWeights[313]				: register(c1336);
//					cFlexWeights[314]				: register(c1337);
//					cFlexWeights[315]				: register(c1338);
//					cFlexWeights[316]				: register(c1339);
//					cFlexWeights[317]				: register(c1340);
//					cFlexWeights[318]				: register(c1341);
//					cFlexWeights[319]				: register(c1342);
//					cFlexWeights[320]				: register(c1343);
//					cFlexWeights[321]				: register(c1344);
//					cFlexWeights[322]				: register(c1345);
//					cFlexWeights[323]				: register(c1346);
//					cFlexWeights[324]				: register(c1347);
//					cFlexWeights[325]				: register(c1348);
//					cFlexWeights[326]				: register(c1349);
//					cFlexWeights[327]				: register(c1350);
//					cFlexWeights[328]				: register(c1351);
//					cFlexWeights[329]				: register(c1352);
//					cFlexWeights[330]				: register(c1353);
//					cFlexWeights[331]				: register(c1354);
//					cFlexWeights[332]				: register(c1355);
//					cFlexWeights[333]				: register(c1356);
//					cFlexWeights[334]				: register(c1357);
//					cFlexWeights[335]				: register(c1358);
//					cFlexWeights[336]				: register(c1359);
//					cFlexWeights[337]				: register(c1360);
//					cFlexWeights[338]				: register(c1361);
//					cFlexWeights[339]				: register(c1362);
//					cFlexWeights[340]				: register(c1363);
//					cFlexWeights[341]				: register(c1364);
//					cFlexWeights[342]				: register(c1365);
//					cFlexWeights[343]				: register(c1366);
//					cFlexWeights[344]				: register(c1367);
//					cFlexWeights[345]				: register(c1368);
//					cFlexWeights[346]				: register(c1369);
//					cFlexWeights[347]				: register(c1370);
//					cFlexWeights[348]				: register(c1371);
//					cFlexWeights[349]				: register(c1372);
//					cFlexWeights[350]				: register(c1373);
//					cFlexWeights[351]				: register(c1374);
//					cFlexWeights[352]				: register(c1375);
//					cFlexWeights[353]				: register(c1376);
//					cFlexWeights[354]				: register(c1377);
//					cFlexWeights[355]				: register(c1378);
//					cFlexWeights[356]				: register(c1379);
//					cFlexWeights[357]				: register(c1380);
//					cFlexWeights[358]				: register(c1381);
//					cFlexWeights[359]				: register(c1382);
//					cFlexWeights[360]				: register(c1383);
//					cFlexWeights[361]				: register(c1384);
//					cFlexWeights[362]				: register(c1385);
//					cFlexWeights[363]				: register(c1386);
//					cFlexWeights[364]				: register(c1387);
//					cFlexWeights[365]				: register(c1388);
//					cFlexWeights[366]				: register(c1389);
//					cFlexWeights[367]				: register(c1390);
//					cFlexWeights[368]				: register(c1391);
//					cFlexWeights[369]				: register(c1392);
//					cFlexWeights[370]				: register(c1393);
//					cFlexWeights[371]				: register(c1394);
//					cFlexWeights[372]				: register(c1395);
//					cFlexWeights[373]				: register(c1396);
//					cFlexWeights[374]				: register(c1397);
//					cFlexWeights[375]				: register(c1398);
//					cFlexWeights[376]				: register(c1399);
//					cFlexWeights[377]				: register(c1400);
//					cFlexWeights[378]				: register(c1401);
//					cFlexWeights[379]				: register(c1402);
//					cFlexWeights[380]				: register(c1403);
//					cFlexWeights[381]				: register(c1404);
//					cFlexWeights[382]				: register(c1405);
//					cFlexWeights[383]				: register(c1406);
//					cFlexWeights[384]				: register(c1407);
//					cFlexWeights[385]				: register(c1408);
//					cFlexWeights[386]				: register(c1409);
//					cFlexWeights[387]				: register(c1410);
//					cFlexWeights[388]				: register(c1411);
//					cFlexWeights[389]				: register(c1412);
//					cFlexWeights[390]				: register(c1413);
//					cFlexWeights[391]				: register(c1414);
//					cFlexWeights[392]				: register(c1415);
//					cFlexWeights[393]				: register(c1416);
//					cFlexWeights[394]				: register(c1417);
//					cFlexWeights[395]				: register(c1418);
//					cFlexWeights[396]				: register(c1419);
//					cFlexWeights[397]				: register(c1420);
//					cFlexWeights[398]				: register(c1421);
//					cFlexWeights[399]				: register(c1422);
//					cFlexWeights[400]				: register(c1423);
//					cFlexWeights[401]				: register(c1424);
//					cFlexWeights[402]				: register(c1425);
//					cFlexWeights[403]				: register(c1426);
//					cFlexWeights[404]				: register(c1427);
//					cFlexWeights[405]				: register(c1428);
//					cFlexWeights[406]				: register(c1429);
//					cFlexWeights[407]				: register(c1430);
//					cFlexWeights[408]				: register(c1431);
//					cFlexWeights[409]				: register(c1432);
//					cFlexWeights[410]				: register(c1433);
//					cFlexWeights[411]				: register(c1434);
//					cFlexWeights[412]				: register(c1435);
//					cFlexWeights[413]				: register(c1436);
//					cFlexWeights[414]				: register(c1437);
//					cFlexWeights[415]				: register(c1438);
//					cFlexWeights[416]				: register(c1439);
//					cFlexWeights[417]				: register(c1440);
//					cFlexWeights[418]				: register(c1441);
//					cFlexWeights[419]				: register(c1442);
//					cFlexWeights[420]				: register(c1443);
//					cFlexWeights[421]				: register(c1444);
//					cFlexWeights[422]				: register(c1445);
//					cFlexWeights[423]				: register(c1446);
//					cFlexWeights[424]				: register(c1447);
//					cFlexWeights[425]				: register(c1448);
//					cFlexWeights[426]				: register(c1449);
//					cFlexWeights[427]				: register(c1450);
//					cFlexWeights[428]				: register(c1451);
//					cFlexWeights[429]				: register(c1452);
//					cFlexWeights[430]				: register(c1453);
//					cFlexWeights[431]				: register(c1454);
//					cFlexWeights[432]				: register(c1455);
//					cFlexWeights[433]				: register(c1456);
//					cFlexWeights[434]				: register(c1457);
//					cFlexWeights[435]				: register(c1458);
//					cFlexWeights[436]				: register(c1459);
//					cFlexWeights[437]				: register(c1460);
//					cFlexWeights[438]				: register(c1461);
//					cFlexWeights[439]				: register(c1462);
//					cFlexWeights[440]				: register(c1463);
//					cFlexWeights[441]				: register(c1464);
//					cFlexWeights[442]				: register(c1465);
//					cFlexWeights[443]				: register(c1466);
//					cFlexWeights[444]				: register(c1467);
//					cFlexWeights[445]				: register(c1468);
//					cFlexWeights[446]				: register(c1469);
//					cFlexWeights[447]				: register(c1470);
//					cFlexWeights[448]				: register(c1471);
//					cFlexWeights[449]				: register(c1472);
//					cFlexWeights[450]				: register(c1473);
//					cFlexWeights[451]				: register(c1474);
//					cFlexWeights[452]				: register(c1475);
//					cFlexWeights[453]				: register(c1476);
//					cFlexWeights[454]				: register(c1477);
//					cFlexWeights[455]				: register(c1478);
//					cFlexWeights[456]				: register(c1479);
//					cFlexWeights[457]				: register(c1480);
//					cFlexWeights[458]				: register(c1481);
//					cFlexWeights[459]				: register(c1482);
//					cFlexWeights[460]				: register(c1483);
//					cFlexWeights[461]				: register(c1484);
//					cFlexWeights[462]				: register(c1485);
//					cFlexWeights[463]				: register(c1486);
//					cFlexWeights[464]				: register(c1487);
//					cFlexWeights[465]				: register(c1488);
//					cFlexWeights[466]				: register(c1489);
//					cFlexWeights[467]				: register(c1490);
//					cFlexWeights[468]				: register(c1491);
//					cFlexWeights[469]				: register(c1492);
//					cFlexWeights[470]				: register(c1493);
//					cFlexWeights[471]				: register(c1494);
//					cFlexWeights[472]				: register(c1495);
//					cFlexWeights[473]				: register(c1496);
//					cFlexWeights[474]				: register(c1497);
//					cFlexWeights[475]				: register(c1498);
//					cFlexWeights[476]				: register(c1499);
//					cFlexWeights[477]				: register(c1500);
//					cFlexWeights[478]				: register(c1501);
//					cFlexWeights[479]				: register(c1502);
//					cFlexWeights[480]				: register(c1503);
//					cFlexWeights[481]				: register(c1504);
//					cFlexWeights[482]				: register(c1505);
//					cFlexWeights[483]				: register(c1506);
//					cFlexWeights[484]				: register(c1507);
//					cFlexWeights[485]				: register(c1508);
//					cFlexWeights[486]				: register(c1509);
//					cFlexWeights[487]				: register(c1510);
//					cFlexWeights[488]				: register(c1511);
//					cFlexWeights[489]				: register(c1512);
//					cFlexWeights[490]				: register(c1513);
//					cFlexWeights[491]				: register(c1514);
//					cFlexWeights[492]				: register(c1515);
//					cFlexWeights[493]				: register(c1516);
//					cFlexWeights[494]				: register(c1517);
//					cFlexWeights[495]				: register(c1518);
//					cFlexWeights[496]				: register(c1519);
//					cFlexWeights[497]				: register(c1520);
//					cFlexWeights[498]				: register(c1521);
//					cFlexWeights[499]				: register(c1522);
//					cFlexWeights[500]				: register(c1523);
//					cFlexWeights[501]				: register(c1524);
//					cFlexWeights[502]				: register(c1525);
//					cFlexWeights[503]				: register(c1526);
//					cFlexWeights[504]				: register(c1527);
//					cFlexWeights[505]				: register(c1528);
//					cFlexWeights[506]				: register(c1529);
//					cFlexWeights[507]				: register(c1530);
//					cFlexWeights[508]				: register(c1531);
//					cFlexWeights[509]				: register(c1532);
//					cFlexWeights[510]				: register(c1533);
//					cFlexWeights[511]				: register(c1534);
#endif
//					TODO: Can we use anything above c1534? I beg not. It took me probably 2 hours to make this list.

// c1
#define cOOGamma			cConstants1.x
#define cOverbright			2.0f
#define cOneThird			cConstants1.z
#define cOOOverbright		( 1.0f / 2.0f )
// c2
#define cEyePos			cEyePosWaterZ.xyz
// c16
#define cFogEndOverFogRange cFogParams.x
#define cFogOne cFogParams.y
#define cFogMaxDensity cFogParams.z
#define cOOFogRange cFogParams.w
// c29.. ( part of cLightInfo, c27 )
#define LIGHT_0_POSITION_REG					   c29
// c58 ... Why is this not a define?
static const int cModel0Index = 58;

//===========================================================================//
//	Declaring VertexShader SAMPLERS
//	Actually we do this on the Shaders themselfs
//	This file will be faster to parse the less stuff is in it.
//===========================================================================//





//===========================================================================//
//	Declaring Functions. This is where it gets interesting...
//===========================================================================//

//===========================================================================//
//	Needed for SkinPositionAndNormal on both models and brushes.
//===========================================================================//
float3 mul4x3(float4 v, float4x3 m)
{
	return float3(dot(v, transpose(m)[0]), dot(v, transpose(m)[1]), dot(v, transpose(m)[2]));
}

// Original Comment for both mul4x3 and mul3x3 in common_fxc.h
// "Versions of matrix multiply functions which force HLSL compiler to explictly use DOTs, 
// not giving it the option of using MAD expansion.  In a perfect world, the compiler would
// always pick the best strategy, and these shouldn't be needed.. but.. well.. umm..
//
// lorenmcq"

//===========================================================================//
//	Needed for SkinPositionAndNormal on both models and brushes.
//===========================================================================//
float3 mul3x3(float3 v, float3x3 m)
{
	return float3(dot(v, transpose(m)[0]), dot(v, transpose(m)[1]), dot(v, transpose(m)[2]));
}

//===========================================================================//
//	Usually the first function to be called from a VS.
//	Used to unpack the Normal from Compressed Data. Only used on models!
//	What? You found it on Brushes? Oh... Well, the damage was already done -.-
//	This function is utterly useless for Brushes. It just returns what was already there.
//	FIXME: Instead of DecompressVertex_Normal(), swivel Normals VS that don't use Vertex-Compression
//===========================================================================//
#if defined(MODEL)
// Optimized version which reads 2 deltas
void SampleMorphDelta2( sampler2D vt, const float3 vMorphTargetTextureDim, const float4 vMorphSubrect, const float flVertexID, out float4 delta1, out float4 delta2 )
{
	float flColumn = floor( flVertexID / vMorphSubrect.w );

	float4 t;
	t.x = vMorphSubrect.x + vMorphTargetTextureDim.z * flColumn + 0.5f;
	t.y = vMorphSubrect.y + flVertexID - flColumn * vMorphSubrect.w + 0.5f;
	t.xy /= vMorphTargetTextureDim.xy;	
	t.z = t.w = 0.f;

	delta1 = tex2Dlod( vt, t );
	t.x += 1.0f / vMorphTargetTextureDim.x;
	delta2 = tex2Dlod( vt, t );
}

//===========================================================================//
//	Needed for SkinPositionAndNormal
//===========================================================================//
float4 DecompressBoneWeights( const float4 weights )
{
	float4 result = weights;

//	if ( COMPRESSED_VERTS ) // Not needed, models always have this.
	{
		// Decompress from SHORT2 to float. In our case, [-1, +32767] -> [0, +1]
		// NOTE: we add 1 here so we can divide by 32768 - which is exact (divide by 32767 is not).
		//       This avoids cracking between meshes with different numbers of bone weights.
		//       We use SHORT2 instead of SHORT2N for a similar reason - the GPU's conversion
		//       from [-32768,+32767] to [-1,+1] is imprecise in the same way.
		result += 1;
		result /= 32768;
	}

	return result;
}

//===========================================================================//
//	Used on models to adjust the actual positions for rendering.
//===========================================================================//
void SkinPositionAndNormal(const bool bSkinning, const float4 modelPos, const float3 modelNormal,
	const float4 boneWeights, float4 fBoneIndices,
	out float3 worldPos, out float3 worldNormal)
{
	int3 boneIndices = D3DCOLORtoUBYTE4(fBoneIndices);

	if (!bSkinning)
	{
		worldPos = mul4x3(modelPos, cModel[0]);
		worldNormal = mul3x3(modelNormal, (const float3x3)cModel[0]);
	}
	else // skinning - always three bones
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float3 weights = DecompressBoneWeights(boneWeights).xyz;
		weights[2] = 1 - (weights[0] + weights[1]);

		float4x3 blendMatrix = mat1 * weights[0] + mat2 * weights[1] + mat3 * weights[2];
		worldPos = mul4x3(modelPos, blendMatrix);
		worldNormal = mul3x3(modelNormal, (float3x3)blendMatrix);
	}
}

//===========================================================================//
// From common_vs_fxc.h, We can't really change Vertex-Compression.
// So we are forced to do exactly what Valve did originally. Which is this function.
// Unmodified function from common_vs_fxc.h
// Btw Vertex Compression is used to reduce the memory bandwith and increase performance.
// How does that work if the function looks like this? IDK
// Original comment :
// "Decompress just a normal from four-component compressed format (same as above)
// We expect this data to come from an unsigned UBYTE4 stream in the range of 0..255
// [ When compiled, this works out to approximately 17 asm instructions ]"
//===========================================================================//
void _DecompressUByte4Normal(float4 inputNormal,
	out float3 outputNormal)					// {nX, nY, nZ}
{
	float fOne = 1.0f;

	float2 ztSigns = (inputNormal.xy - 128.0f) < 0;				// sign bits for zs and binormal (1 or 0)  set-less-than (slt) asm instruction
	float2 xyAbs = abs(inputNormal.xy - 128.0f) - ztSigns;		// 0..127
	float2 xySigns = (xyAbs - 64.0f) < 0;						// sign bits for xs and ys (1 or 0)
	outputNormal.xy = (abs(xyAbs - 64.0f) - xySigns) / 63.0f;	// abs({nX, nY})

	outputNormal.z = 1.0f - outputNormal.x - outputNormal.y;		// Project onto x+y+z=1
	outputNormal.xyz = normalize(outputNormal.xyz);				// Normalize onto unit sphere

	outputNormal.xy *= lerp(fOne.xx, -fOne.xx, xySigns);			// Restore x and y signs
	outputNormal.z *= lerp(fOne.x, -fOne.x, ztSigns.x);			// Restore z sign
}

// I had a talk with Tottery and apparently Models always use Vertex Compression.
// So RIP two useless if statements!
void DecompressVertex_Normal(float4 f4InputNormal, out float3 f3OutputNormal)
{
//	if (COMPRESSED_VERTS == 1)
//	{
		_DecompressUByte4Normal(f4InputNormal, f3OutputNormal);
//	}
//	else
//	{
//		f3OutputNormal = f4InputNormal.xyz;
//	}
}
#else
void SkinPositionAndNormal(const bool bDummy, const float4 modelPos, const float3 modelNormal,
	const float4 boneWeights, float4 fBoneIndices,
	out float3 worldPos, out float3 worldNormal)
{
	int3 boneIndices = D3DCOLORtoUBYTE4(fBoneIndices);

	// Brushes don't have skinning
	worldPos = mul4x3(modelPos, cModel[0]);
	worldNormal = mul3x3(modelNormal, (const float3x3)cModel[0]);
}

void DecompressVertex_Normal(float4 f4InputNormal, out float3 f3OutputNormal)
{
	f3OutputNormal = f4InputNormal.xyz;
}
#endif

//===========================================================================//
//	Function to compute the Attenuation for a given Light.
//	Used for Bumped Lighting *and* Vertex Lighting!
//===========================================================================//
float LUX_GetLightAttenuation(const float3 f3WorldPos, int iLight)
{
	float f1Result = 0.0f;

	// Can't use Static-Control-Flow, compiler will complain to us that we are using them incorrectly...
	// if(g_bLightEnabled[iLight])

	// Get light direction
	float3 f3LightDir = cLightInfo[iLight].pos - f3WorldPos;

	// Get light distance squared.
	float f1LightDistSquared = dot(f3LightDir, f3LightDir);

	// Get 1/lightDistance
	float f1LightDist = rsqrt(f1LightDistSquared); // rsqrt() = Returns the reciprocal of the square root of the specified value

	// Get Light Direction to -1..1 range
	f3LightDir *= f1LightDist;
	float3 f3Dist = dst(f1LightDistSquared, f1LightDist);

	float f1DistanceAtten = 1.0f / dot(cLightInfo[iLight].atten.xyz, f3Dist);

	// Spot Attenuation
	// [TODO] Shouldn't we check if the light we are calculating is from a spotlight to begin with?
	// {
	float f1CosTheta = dot(cLightInfo[iLight].dir.xyz, -f3LightDir);
	float f1SpotAtten = (f1CosTheta - cLightInfo[iLight].spotParams.z) * cLightInfo[iLight].spotParams.w;
	f1SpotAtten = max(0.0001f, f1SpotAtten);
	f1SpotAtten = pow(f1SpotAtten, cLightInfo[iLight].spotParams.x);
	f1SpotAtten = saturate(f1SpotAtten);
	// }

	// Select between point and spot
	float f1Atten = lerp(f1DistanceAtten, f1DistanceAtten * f1SpotAtten, cLightInfo[iLight].dir.w);

	// Select between above and directional (no attenuation)
	f1Result = lerp(f1Atten, 1.0f, cLightInfo[iLight].color.w);

	return f1Result;
}


//===========================================================================//
//	Returns AmbientCube's Color.
//	Mostly unchanged from the original function found in common_vs_fxc.h
//===========================================================================//
float3 LUX_AmbientCube(const float3 f3WorldNormal)
{
	float3 nSquared = f3WorldNormal * f3WorldNormal;
	int3 isNegative = (f3WorldNormal < 0.0);
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCubeX[isNegative.x] +
		nSquared.y * cAmbientCubeY[isNegative.y] +
		nSquared.z * cAmbientCubeZ[isNegative.z];
	return linearColor;
}

//===========================================================================//
//	Helper-Function
//===========================================================================//
float LUX_CosineTermInternal(const float3 f3WorldPos, const float3 f3WorldNormal, int iLight, const bool bHalfLambert)
{
	// Calculate light direction assuming this is a point or spot
	float3 f3LightDir = normalize(cLightInfo[iLight].pos - f3WorldPos);

	// Select the above direction or the one in the structure, based upon light type
	f3LightDir = lerp(f3LightDir, -cLightInfo[iLight].dir, cLightInfo[iLight].color.w);

	// compute N dot L
	float NDotL = dot(f3WorldNormal, f3LightDir);

	if (!bHalfLambert)
	{
		NDotL = max(0.0f, NDotL);
	}
	else	// Half-Lambert
	{
		NDotL = NDotL * 0.5 + 0.5;
		NDotL = NDotL * NDotL;
	}
	return NDotL;
}

//===========================================================================//
//	Helper-Function
//===========================================================================//
float3 LUX_DoLightInternal(const float3 worldPos, const float3 worldNormal, int lightNum, bool bHalfLambert) // this used to just be a bool despite HalfLambert being a Static Combo.
{
	return cLightInfo[lightNum].color *
		LUX_CosineTermInternal(worldPos, worldNormal, lightNum, bHalfLambert) *
		LUX_GetLightAttenuation(worldPos, lightNum);
}

//===========================================================================//
//	Computes Lighting for Static, Dynamic, Physics props with no bumpmapping.
//===========================================================================//
float3 LUX_VertexLighting(const float3 f3WorldPos, const float3 f3WorldNormal,
	const float3 f3StaticLightingColor, const bool bStaticLight,
	const bool bDynamicLight, bool bHalfLambert) // this used to just be a bool despite HalfLambert being a Static Combo.
{
	float3 f3LinearColor = float3(0.0f, 0.0f, 0.0f);
	if (bStaticLight) // -StaticPropLighting
	{
		float3 f3StaticLight = f3StaticLightingColor * cOverbright;
		f3LinearColor += pow(f3StaticLight, 2.2f); // Gamma to Linear
	}

	if (bDynamicLight) // Dynamic lighting via toggleable-Lights or on dynamic objects
	{
		// Integer LightCount can only be used for for()-Loops!
		for (int i = 0; i < g_nLightCount; i++)
		{
			f3LinearColor += LUX_DoLightInternal(f3WorldPos, f3WorldNormal, i, bHalfLambert);
			// TODO: This might not work for Static Props... Investigate!
			// If it doesn't, exclude to else statement of if(bStaticLight)
			f3LinearColor += LUX_AmbientCube(f3WorldNormal); //ambient light is already remapped
		}
	}
	return f3LinearColor;
}

#endif //#ifndef LUX_COMMON_VS_FXC_H_
