/********************************************************************************
*                                                                               *
*                                  Vector test                                  *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2008 by Niall Douglas.   All Rights Reserved.            *
*   NOTE THAT I NIALL DOUGLAS DO NOT PERMIT ANY OF MY CODE USED UNDER THE GPL   *
*********************************************************************************
* This code is free software; you can redistribute it and/or modify it under    *
* the terms of the GNU Library General Public License v2.1 as published by the  *
* Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
* "upgrade" this code to the GPL without my prior written permission.           *
* Please consult the file "License_Addendum2.txt" accompanying this file.       *
*                                                                               *
* This code is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
*********************************************************************************
* $Id:                                                                          *
********************************************************************************/

#include "FXMaths.h"
using namespace FX;
using namespace FX::Maths;

static const FXuint MersenneTwisterProof[]={
2920711183U, 3885745737U, 3501893680U,  856470934U, 1421864068U, 
 277361036U, 1518638004U, 2328404353U, 3355513634U,   64329189U, 
1624587673U, 3508467182U, 2481792141U, 3706480799U, 1925859037U, 
2913275699U,  882658412U,  384641219U,  422202002U, 1873384891U, 
2006084383U, 3924929912U, 1636718106U, 3108838742U, 1245465724U, 
4195470535U,  779207191U, 1577721373U, 1390469554U, 2928648150U, 
 121399709U, 3170839019U, 4044347501U,  953953814U, 3821710850U, 
3085591323U, 3666535579U, 3577837737U, 2012008410U, 3565417471U, 
4044408017U,  433600965U, 1637785608U, 1798509764U,  860770589U, 
3081466273U, 3982393409U, 2451928325U, 3437124742U, 4093828739U, 
3357389386U, 2154596123U,  496568176U, 2650035164U, 2472361850U, 
   3438299U, 2150366101U, 1577256676U, 3802546413U, 1787774626U, 
4078331588U, 3706103141U,  170391138U, 3806085154U, 1680970100U, 
1961637521U, 3316029766U,  890610272U, 1453751581U, 1430283664U, 
3051057411U, 3597003186U,  542563954U, 3796490244U, 1690016688U, 
3448752238U,  440702173U,  347290497U, 1121336647U, 2540588620U, 
 280881896U, 2495136428U,  213707396U,   15104824U, 2946180358U, 
 659000016U,  566379385U, 2614030979U, 2855760170U,  334526548U, 
2315569495U, 2729518615U,  564745877U, 1263517638U, 3157185798U, 
1604852056U, 1011639885U, 2950579535U, 2524219188U,  312951012U, 
1528896652U, 1327861054U, 2846910138U, 3966855905U, 2536721582U, 
 855353911U, 1685434729U, 3303978929U, 1624872055U, 4020329649U, 
3164802143U, 1642802700U, 1957727869U, 1792352426U, 3334618929U, 
2631577923U, 3027156164U,  842334259U, 3353446843U, 1226432104U, 
1742801369U, 3552852535U, 3471698828U, 1653910186U, 3380330939U, 
2313782701U, 3351007196U, 2129839995U, 1800682418U, 4085884420U, 
1625156629U, 3669701987U,  615211810U, 3294791649U, 4131143784U, 
2590843588U, 3207422808U, 3275066464U,  561592872U, 3957205738U, 
3396578098U,   48410678U, 3505556445U, 1005764855U, 3920606528U, 
2936980473U, 2378918600U, 2404449845U, 1649515163U,  701203563U, 
3705256349U,   83714199U, 3586854132U,  922978446U, 2863406304U, 
3523398907U, 2606864832U, 2385399361U, 3171757816U, 4262841009U, 
3645837721U, 1169579486U, 3666433897U, 3174689479U, 1457866976U, 
3803895110U, 3346639145U, 1907224409U, 1978473712U, 1036712794U, 
 980754888U, 1302782359U, 1765252468U,  459245755U, 3728923860U, 
1512894209U, 2046491914U,  207860527U,  514188684U, 2288713615U, 
1597354672U, 3349636117U, 2357291114U, 3995796221U,  945364213U, 
1893326518U, 3770814016U, 1691552714U, 2397527410U,  967486361U, 
 776416472U, 4197661421U,  951150819U, 1852770983U, 4044624181U, 
1399439738U, 4194455275U, 2284037669U, 1550734958U, 3321078108U, 
1865235926U, 2912129961U, 2664980877U, 1357572033U, 2600196436U, 
2486728200U, 2372668724U, 1567316966U, 2374111491U, 1839843570U, 
  20815612U, 3727008608U, 3871996229U,  824061249U, 1932503978U, 
3404541726U,  758428924U, 2609331364U, 1223966026U, 1299179808U, 
 648499352U, 2180134401U,  880821170U, 3781130950U,  113491270U, 
1032413764U, 4185884695U, 2490396037U, 1201932817U, 4060951446U, 
4165586898U, 1629813212U, 2887821158U,  415045333U,  628926856U, 
2193466079U, 3391843445U, 2227540681U, 1907099846U, 2848448395U, 
1717828221U, 1372704537U, 1707549841U, 2294058813U, 2101214437U, 
2052479531U, 1695809164U, 3176587306U, 2632770465U,   81634404U, 
1603220563U,  644238487U,  302857763U,  897352968U, 2613146653U, 
1391730149U, 4245717312U, 4191828749U, 1948492526U, 2618174230U, 
3992984522U, 2178852787U, 3596044509U, 3445573503U, 2026614616U, 
 915763564U, 3415689334U, 2532153403U, 3879661562U, 2215027417U, 
3111154986U, 2929478371U,  668346391U, 1152241381U, 2632029711U, 
3004150659U, 2135025926U,  948690501U, 2799119116U, 4228829406U, 
1981197489U, 4209064138U,  684318751U, 3459397845U,  201790843U, 
4022541136U, 3043635877U,  492509624U, 3263466772U, 1509148086U, 
 921459029U, 3198857146U,  705479721U, 3835966910U, 3603356465U, 
 576159741U, 1742849431U,  594214882U, 2055294343U, 3634861861U, 
 449571793U, 3246390646U, 3868232151U, 1479156585U, 2900125656U, 
2464815318U, 3960178104U, 1784261920U,   18311476U, 3627135050U, 
 644609697U,  424968996U,  919890700U, 2986824110U,  816423214U, 
4003562844U, 1392714305U, 1757384428U, 2569030598U,  995949559U, 
3875659880U, 2933807823U, 2752536860U, 2993858466U, 4030558899U, 
2770783427U, 2775406005U, 2777781742U, 1931292655U,  472147933U, 
3865853827U, 2726470545U, 2668412860U, 2887008249U,  408979190U, 
3578063323U, 3242082049U, 1778193530U,   27981909U, 2362826515U, 
 389875677U, 1043878156U,  581653903U, 3830568952U,  389535942U, 
3713523185U, 2768373359U, 2526101582U, 1998618197U, 1160859704U, 
3951172488U, 1098005003U,  906275699U, 3446228002U, 2220677963U, 
2059306445U,  132199571U,  476838790U, 1868039399U, 3097344807U, 
 857300945U,  396345050U, 2835919916U, 1782168828U, 1419519470U, 
4288137521U,  819087232U,  596301494U,  872823172U, 1526888217U, 
 805161465U, 1116186205U, 2829002754U, 2352620120U,  620121516U, 
 354159268U, 3601949785U,  209568138U, 1352371732U, 2145977349U, 
4236871834U, 1539414078U, 3558126206U, 3224857093U, 4164166682U, 
3817553440U, 3301780278U, 2682696837U, 3734994768U, 1370950260U, 
1477421202U, 2521315749U, 1330148125U, 1261554731U, 2769143688U, 
3554756293U, 4235882678U, 3254686059U, 3530579953U, 1215452615U, 
3574970923U, 4057131421U,  589224178U, 1000098193U,  171190718U, 
2521852045U, 2351447494U, 2284441580U, 2646685513U, 3486933563U, 
3789864960U, 1190528160U, 1702536782U, 1534105589U, 4262946827U, 
2726686826U, 3584544841U, 2348270128U, 2145092281U, 2502718509U, 
1027832411U, 3571171153U, 1287361161U, 4011474411U, 3241215351U, 
2419700818U,  971242709U, 1361975763U, 1096842482U, 3271045537U, 
  81165449U,  612438025U, 3912966678U, 1356929810U,  733545735U, 
 537003843U, 1282953084U,  884458241U,  588930090U, 3930269801U, 
2961472450U, 1219535534U, 3632251943U,  268183903U, 1441240533U, 
3653903360U, 3854473319U, 2259087390U, 2548293048U, 2022641195U, 
2105543911U, 1764085217U, 3246183186U,  482438805U,  888317895U, 
2628314765U, 2466219854U,  717546004U, 2322237039U,  416725234U, 
1544049923U, 1797944973U, 3398652364U, 3111909456U,  485742908U, 
2277491072U, 1056355088U, 3181001278U,  129695079U, 2693624550U, 
1764438564U, 3797785470U,  195503713U, 3266519725U, 2053389444U, 
1961527818U, 3400226523U, 3777903038U, 2597274307U, 4235851091U, 
4094406648U, 2171410785U, 1781151386U, 1378577117U,  654643266U, 
3424024173U, 3385813322U,  679385799U,  479380913U,  681715441U, 
3096225905U,  276813409U, 3854398070U, 2721105350U,  831263315U, 
3276280337U, 2628301522U, 3984868494U, 1466099834U, 2104922114U, 
1412672743U,  820330404U, 3491501010U,  942735832U,  710652807U, 
3972652090U,  679881088U,   40577009U, 3705286397U, 2815423480U, 
3566262429U,  663396513U, 3777887429U, 4016670678U,  404539370U, 
1142712925U, 1140173408U, 2913248352U, 2872321286U,  263751841U, 
3175196073U, 3162557581U, 2878996619U,   75498548U, 3836833140U, 
3284664959U, 1157523805U,  112847376U,  207855609U, 1337979698U, 
1222578451U,  157107174U,  901174378U, 3883717063U, 1618632639U, 
1767889440U, 4264698824U, 1582999313U,  884471997U, 2508825098U, 
3756370771U, 2457213553U, 3565776881U, 3709583214U,  915609601U, 
 460833524U, 1091049576U,   85522880U,    2553251U,  132102809U, 
2429882442U, 2562084610U, 1386507633U, 4112471229U,   21965213U, 
1981516006U, 2418435617U, 3054872091U, 4251511224U, 2025783543U, 
1916911512U, 2454491136U, 3938440891U, 3825869115U, 1121698605U, 
3463052265U,  802340101U, 1912886800U, 4031997367U, 3550640406U, 
1596096923U,  610150600U,  431464457U, 2541325046U,  486478003U, 
 739704936U, 2862696430U, 3037903166U, 1129749694U, 2611481261U, 
1228993498U,  510075548U, 3424962587U, 2458689681U,  818934833U, 
4233309125U, 1608196251U, 3419476016U, 1858543939U, 2682166524U, 
3317854285U,  631986188U, 3008214764U,  613826412U, 3567358221U, 
3512343882U, 1552467474U, 3316162670U, 1275841024U, 4142173454U, 
 565267881U,  768644821U,  198310105U, 2396688616U, 1837659011U, 
 203429334U,  854539004U, 4235811518U, 3338304926U, 3730418692U, 
3852254981U, 3032046452U, 2329811860U, 2303590566U, 2696092212U, 
3894665932U,  145835667U,  249563655U, 1932210840U, 2431696407U, 
3312636759U,  214962629U, 2092026914U, 3020145527U, 4073039873U, 
2739105705U, 1308336752U,  855104522U, 2391715321U,   67448785U, 
 547989482U,  854411802U, 3608633740U,  431731530U,  537375589U, 
3888005760U,  696099141U,  397343236U, 1864511780U,   44029739U, 
1729526891U, 1993398655U, 2010173426U, 2591546756U,  275223291U, 
1503900299U, 4217765081U, 2185635252U, 1122436015U, 3550155364U, 
 681707194U, 3260479338U,  933579397U, 2983029282U, 2505504587U, 
2667410393U, 2962684490U, 4139721708U, 2658172284U, 2452602383U, 
2607631612U, 1344296217U, 3075398709U, 2949785295U, 1049956168U, 
3917185129U, 2155660174U, 3280524475U, 1503827867U,  674380765U, 
1918468193U, 3843983676U,  634358221U, 2538335643U, 1873351298U, 
3368723763U, 2129144130U, 3203528633U, 3087174986U, 2691698871U, 
2516284287U,   24437745U, 1118381474U, 2816314867U, 2448576035U, 
4281989654U,  217287825U,  165872888U, 2628995722U, 3533525116U, 
2721669106U,  872340568U, 3429930655U, 3309047304U, 3916704967U, 
3270160355U, 1348884255U, 1634797670U,  881214967U, 4259633554U, 
 174613027U, 1103974314U, 1625224232U, 2678368291U, 1133866707U, 
3853082619U, 4073196549U, 1189620777U,  637238656U,  930241537U, 
4042750792U, 3842136042U, 2417007212U, 2524907510U, 1243036827U, 
1282059441U, 3764588774U, 1394459615U, 2323620015U, 1166152231U, 
3307479609U, 3849322257U, 3507445699U, 4247696636U,  758393720U, 
 967665141U, 1095244571U, 1319812152U,  407678762U, 2640605208U, 
2170766134U, 3663594275U, 4039329364U, 2512175520U,  725523154U, 
2249807004U, 3312617979U, 2414634172U, 1278482215U,  349206484U, 
1573063308U, 1196429124U, 3873264116U, 2400067801U,  268795167U, 
 226175489U, 2961367263U, 1968719665U,   42656370U, 1010790699U, 
 561600615U, 2422453992U, 3082197735U, 1636700484U, 3977715296U, 
3125350482U, 3478021514U, 2227819446U, 1540868045U, 3061908980U, 
1087362407U, 3625200291U,  361937537U,  580441897U, 1520043666U, 
2270875402U, 1009161260U, 2502355842U, 4278769785U,  473902412U, 
1057239083U, 1905829039U, 1483781177U, 2080011417U, 1207494246U, 
1806991954U, 2194674403U, 3455972205U,  807207678U, 3655655687U, 
 674112918U,  195425752U, 3917890095U, 1874364234U, 1837892715U, 
3663478166U, 1548892014U, 2570748714U, 2049929836U, 2167029704U, 
 697543767U, 3499545023U, 3342496315U, 1725251190U, 3561387469U, 
2905606616U, 1580182447U, 3934525927U, 4103172792U, 1365672522U, 
1534795737U, 3308667416U, 2841911405U, 3943182730U, 4072020313U, 
3494770452U, 3332626671U,   55327267U,  478030603U,  411080625U, 
3419529010U, 1604767823U, 3513468014U,  570668510U,  913790824U, 
2283967995U,  695159462U, 3825542932U, 4150698144U, 1829758699U, 
 202895590U, 1609122645U, 1267651008U, 2910315509U, 2511475445U, 
2477423819U, 3932081579U,  900879979U, 2145588390U, 2670007504U, 
 580819444U, 1864996828U, 2526325979U, 1019124258U,  815508628U, 
2765933989U, 1277301341U, 3006021786U,  855540956U,  288025710U, 
1919594237U, 2331223864U,  177452412U, 2475870369U, 2689291749U, 
 865194284U,  253432152U, 2628531804U, 2861208555U, 2361597573U, 
1653952120U, 1039661024U, 2159959078U, 3709040440U, 3564718533U, 
2596878672U, 2041442161U,   31164696U, 2662962485U, 3665637339U, 
1678115244U, 2699839832U, 3651968520U, 3521595541U,  458433303U, 
2423096824U,   21831741U,  380011703U, 2498168716U,  861806087U, 
1673574843U, 4188794405U, 2520563651U, 2632279153U, 2170465525U, 
4171949898U, 3886039621U, 1661344005U, 3424285243U,  992588372U, 
2500984144U, 2993248497U, 3590193895U, 1535327365U,  515645636U, 
 131633450U, 3729760261U, 1613045101U, 3254194278U,   15889678U, 
1493590689U,  244148718U, 2991472662U, 1401629333U,  777349878U, 
2501401703U, 4285518317U, 3794656178U,  955526526U, 3442142820U, 
3970298374U,  736025417U, 2737370764U, 1271509744U,  440570731U, 
 136141826U, 1596189518U,  923399175U,  257541519U, 3505774281U, 
2194358432U, 2518162991U, 1379893637U, 2667767062U, 3748146247U, 
1821712620U, 3923161384U, 1947811444U, 2392527197U, 4127419685U, 
1423694998U, 4156576871U, 1382885582U, 3420127279U, 3617499534U, 
2994377493U, 4038063986U, 1918458672U, 2983166794U, 4200449033U, 
 353294540U, 1609232588U,  243926648U, 2332803291U,  507996832U, 
2392838793U, 4075145196U, 2060984340U, 4287475136U,   88232602U, 
2491531140U, 4159725633U, 2272075455U,  759298618U,  201384554U, 
 838356250U, 1416268324U,  674476934U,   90795364U,  141672229U, 
3660399588U, 4196417251U, 3249270244U, 3774530247U,   59587265U, 
3683164208U,   19392575U, 1463123697U, 1882205379U,  293780489U, 
2553160622U, 2933904694U,  675638239U, 2851336944U, 1435238743U, 
2448730183U,  804436302U, 2119845972U,  322560608U, 4097732704U, 
2987802540U,  641492617U, 2575442710U, 4217822703U, 3271835300U, 
2836418300U, 3739921620U, 2138378768U, 2879771855U, 4294903423U, 
3121097946U, 2603440486U, 2560820391U, 1012930944U, 2313499967U, 
 584489368U, 3431165766U,  897384869U, 2062537737U, 2847889234U, 
3742362450U, 2951174585U, 4204621084U, 1109373893U, 3668075775U, 
2750138839U, 3518055702U,  733072558U, 4169325400U,  788493625U
};

// 72,000 floats is ~282Kb. Ensure six of these stay inside a L2 cache!
static const FXuint TOTALITEMS=72000;
namespace FP
{
	typedef Array<Vector<float, 18>, TOTALITEMS/18> SlowArray;
	typedef Array<Vector<float, 16>, TOTALITEMS/16> FastArray;
	static SlowArray slowA, slowB, slowC;
	static FastArray fastA, fastB, fastC;
}
namespace Int
{
	typedef Array<Vector<int, 18>, TOTALITEMS/18> SlowArray;
	typedef Array<Vector<int, 16>, TOTALITEMS/16> FastArray;
	static SlowArray slowA, slowB, slowC;
	static FastArray fastA, fastB, fastC;
}

template<class type> static void fill(type *FXRESTRICT array, typename type::value_type::TYPE *FXRESTRICT what)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::value_type); n++)
		{
			(*array)[n]=typename type::value_type(what);
		}
	}
}

template<class type> static void sqrtmulacc(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::value_type); n++)
		{
			typename type::value_type &d=(*dest)[n], &a=(*A)[n], &b=(*B)[n];
			d=sqrt(d+a*b);
		}
	}
}

template<class type> static void acc(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::value_type); n++)
		{
			typename type::value_type &d=(*dest)[n], &a=(*A)[n], &b=(*B)[n];
			d=d+a+b;
		}
	}
}

template<class type> static void dotsum(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::value_type); n++)
		{
			(*dest)[n].set(0, dot((*A)[n], (*B)[n]));
			(*dest)[n].set(1, sum((*A)[n])+sum((*B)[n]));
		}
	}
}

template<class type> static void logic(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::value_type); n++)
		{
			(*dest)[n]=(*dest)[n] ^ ((*A)[n]|(*B)[n]);
		}
	}
}

template<class type> static void compare(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::value_type); n++)
		{
			(*dest)[n]=(*dest)[n] >= (*A)[n] || (*dest)[n] <= (*B)[n] || !((*dest)[n] != (*A)[n]);
		}
	}
}

template<class type> static void matrixmult(type *FXRESTRICT A, type *FXRESTRICT B, type *FXRESTRICT C)
{
#ifndef DEBUG
	for(FXuint a=0; a<10000; a++)
#endif
	{
		(*A)=(*B)*(*C);
	}
}

static double Time(const char *desc, const Generic::BoundFunctorV &_routine)
{
	Generic::BoundFunctorV &routine=(Generic::BoundFunctorV &) _routine;
	FXulong start=FXProcess::getNsCount();
	for(FXuint a=0; a<1000; a++)
		routine();
	FXulong end=FXProcess::getNsCount();
	double ret=65536000000000.0/(end-start);
	fxmessage("%s performed %f ops/sec\n", desc, ret);
	return ret;
}

template<typename A, typename B> static bool verify(A &a, B &b)
{
	int z=TOTALITEMS;
	for(FXuint n=0; n<TOTALITEMS/18; n++)
		if(a[n][0]!=b[n][0] || a[n][1]!=b[n][1] || a[n][2]!=b[n][2] || a[n][3]!=b[n][3])
		{
			fxmessage("Failed at index %u:\n%f, %f, %f, %f\n%f, %f, %f, %f\n", n,
				(double) a[n][0], (double) a[n][1], (double) a[n][2], (double) a[n][3],
				(double) b[n][0], (double) b[n][1], (double) b[n][2], (double) b[n][3]);
			return false;
		}
	return true;
}

int main( int argc, char *argv[] )
{
	double slowt, fastt;
	FXProcess myprocess(argc, argv);

	fxmessage("Floating-Point SIMD vector tests\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
	if(1)
	{	// FP tests
		using namespace FP;
		float foo1[]={ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 0, 0 };
		float foo2[]={ 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 0.0f, 0.0f };
		fxmessage("sizeof(Vector<float, 18>)=%d\n", (int)sizeof(SlowArray::value_type));
		fxmessage("sizeof(Vector<float, 16>)=%d\n", (int)sizeof(FastArray::value_type));
		if(sizeof(SlowArray::value_type)!=18*4) fxerror("sizeof(Vector<float, 18>) is not 72 bytes!\n");
		if(sizeof(FastArray::value_type)!=16*4) fxerror("sizeof(Vector<float, 16>) is not 64 bytes!\n");
		fill(&slowA, foo1);
		fill(&fastA, foo1);
		fill(&slowB, foo1);
		fill(&fastB, foo1);
		fastt=Time("   vector<16>", Generic::BindFunc(fill<FastArray>, &fastC, foo2));
		slowt=Time("   vector<18>", Generic::BindFunc(fill<SlowArray>, &slowC, foo2));
		if(!verify(slowA, fastA)) { fxmessage("Buffer contents not the same!\n"); return 1; }
		if(!verify(slowB, fastB)) { fxmessage("Buffer contents not the same!\n"); return 1; }
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same!\n"); return 1; }
		fxmessage("Filling vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Arithmetic test
		fastt=Time("   vector<16>", Generic::BindFunc(sqrtmulacc<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(sqrtmulacc<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Arithmetic failed!\n"); return 1; }
		fxmessage("Square-Root of Multiply-Accumulate vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Comparison test
		fastt=Time("   vector<16>", Generic::BindFunc(compare<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(compare<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		fxmessage("Comparisons vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Dot & sum test
		fill(&slowA, foo1);
		fill(&fastA, foo1);
		fill(&slowB, foo2);
		fill(&fastB, foo2);
		fill(&slowC, foo1);
		fill(&fastC, foo1);
		fastt=Time("   vector<16>", Generic::BindFunc(dotsum<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(dotsum<SlowArray>, &slowC, &slowA, &slowB));
		const float sC[]={ 210.799988f, 187.000000f, 4.000000f, 5.000000f };
		const float fC[]={ 178.399994f, 167.199997f, 4.000000f, 5.000000f };
		if(slowC[0][0]!=sC[0] || slowC[0][1]!=sC[1] || fastC[0][0]!=fC[0] || fastC[0][1]!=fC[1]) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		fxmessage("Dotsum vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);
	}
	fxmessage("\n\nInteger SIMD vector tests\n"
		          "-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	if(1)
	{	// Logical test
		using namespace Int;
		int foo1[]={ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
		int foo2[]={ 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119 };
		fxmessage("sizeof(Vector<int, 18>)=%d\n", (int)sizeof(SlowArray::value_type));
		fxmessage("sizeof(Vector<int, 16>)=%d\n", (int)sizeof(FastArray::value_type));
		if(sizeof(SlowArray::value_type)!=18*4) fxerror("sizeof(Vector<int, 18>) is not 72 bytes!\n");
		if(sizeof(FastArray::value_type)!=16*4) fxerror("sizeof(Vector<int, 16>) is not 64 bytes!\n");
		fill(&slowA, foo1);
		fill(&fastA, foo1);
		fill(&slowB, foo1);
		fill(&fastB, foo1);
		fastt=Time("   vector<16>", Generic::BindFunc(fill<FastArray>, &fastC, foo2));
		slowt=Time("   vector<18>", Generic::BindFunc(fill<SlowArray>, &slowC, foo2));
		fxmessage("Filling vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Arithmetic test
		fastt=Time("   vector<16>", Generic::BindFunc(acc<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(acc<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Arithmetic failed!\n"); return 1; }
		fxmessage("Accumulate vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Comparison test
		fastt=Time("   vector<16>", Generic::BindFunc(compare<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(compare<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		fxmessage("Comparisons vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		fastt=Time("   vector<16>", Generic::BindFunc(logic<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(logic<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Logical failed!\n"); return 1; }
		fxmessage("Logic vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);
	}
	fxmessage("Floating-Point SIMD matrix tests\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
	if(1)
	{
		float foo1[4][4]={ {0.2f, 0.3f, 0.4f, 0.5f}, {0.6f, 0.7f, 0.8f, 0.9f}, {1.0f, 1.1f, 1.2f, 1.3f}, {1.4f, 1.5f, 1.6f, 1.7f} };
		float foo2[4][4]={ {2.0f, 3.0f, 4.0f, 5.0f}, {6.0f, 7.0f, 8.0f, 9.0f}, {10.0f, 11.0f, 12.0f, 13.0f}, {14.0f, 15.0f, 16.0f, 17.0f} };
		float (*foo)[4]=foo2;
		FXMat4f slowA, slowB, slowC;
		Matrix4f fastA(foo1), fastB(foo2), fastC;
		slowA=fastA;
		slowB=fastB;
		if(Matrix4f(slowA)!=fastA || Matrix4f(slowB)!=fastB) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		//assert(Matrix4f(slowB.transpose())==transpose(fastB));
		fastt=Time("   vector<4>", Generic::BindFunc(matrixmult<Matrix4f>, &fastC, &fastA, &fastB));
		slowt=Time("     FXMat4f", Generic::BindFunc(matrixmult<FXMat4f>, &slowC, &slowA, &slowB));
		if(Matrix4f(slowC)!=fastC)
		{
			fxmessage("Buffer contents not the same! Comparisons failed!\n");
			for(int b=0; b<4; b++) for(int a=0; a<4; a++)
				fxmessage("%c%f", (a&3) ? ' ' : 10, slowC[b][a]);
			fxmessage("\n");
			for(int a=0; a<16; a++)
				fxmessage("%c%f", (a&3) ? ' ' : 10, fastC[a]);
			fxmessage("\n");
			return 1;
		}
		fxmessage("Matrix multiply vector<4> was %f times faster than FXMat4\n\n", fastt/slowt);
	}

	fxmessage("\n\nMaths::FRandomness test\n"
		          "-=-=-=-=-=-=-=-=-=-=-=-\n");
	if(FRandomness::usingSIMD)
	{
		FXuint ini[4] = {0x1234, 0x5678, 0x9abc, 0xdef0};
		FRandomness rand((FXuchar *)ini, 16);
		FXMEMALIGNED(16) FXulong array[500];
		FXulong a;
		rand.fill((FXuchar *) array, sizeof(array));
		for (FXuint i = 0; i < 1000; i+=2) {
			a=array[i/2]; //rand.int64();
			//fxmessage("%u %u ", (FXuint)(a&0xffffffff), (FXuint)((a>>32)&0xffffffff));
			if((FXuint)(a&0xffffffff)!=MersenneTwisterProof[i] || (FXuint)((a>>32)&0xffffffff)!=MersenneTwisterProof[i+1])
			{
				fxmessage("Mersenne twister did not produce correct output!\n");
				return 1;
			}
		}
		fxmessage("Mersenne twister is working correctly!\n");
	}
	{
		FRandomness rand(1);
		FXulong *data=(FXulong *) &Int::fastA;
		FXulong start=FXProcess::getNsCount();
		for(FXuint a=0; a<1024*20; a++)
			rand.fill((FXuchar *) data, sizeof(Int::fastA));
			//for(FXuint n=0; n<TOTALITEMS/2; n++)
			//	data[n]=rand.int64();
		FXulong end=FXProcess::getNsCount();
		fxmessage("Can generate %fMb/sec of FRandomness\n", (sizeof(Int::fastA)*20.0/1024)/((end-start)/1000000000.0));
	}

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	if(!myprocess.isAutomatedTest())
		getchar();
#endif
	return 0;
}
