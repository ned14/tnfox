/* Automatically generated.  Do not edit */
/* See the mkopcodeh.awk script for details */
#define OP_NotExists                            1
#define OP_Dup                                  2
#define OP_AggFunc                              3
#define OP_MoveLt                               4
#define OP_Multiply                            83   /* same as TK_STAR     */
#define OP_BitAnd                              77   /* same as TK_BITAND   */
#define OP_DropTrigger                          5
#define OP_OpenPseudo                           6
#define OP_IntegrityCk                          7
#define OP_RowKey                               8
#define OP_IdxGT                                9
#define OP_Last                                10
#define OP_Subtract                            82   /* same as TK_MINUS    */
#define OP_MemLoad                             11
#define OP_Remainder                           85   /* same as TK_REM      */
#define OP_SetCookie                           12
#define OP_AggSet                              13
#define OP_Pull                                14
#define OP_DropTable                           15
#define OP_MemStore                            16
#define OP_ContextPush                         17
#define OP_IdxIsNull                           18
#define OP_NotNull                             69   /* same as TK_NOTNULL  */
#define OP_Rowid                               19
#define OP_Real                               128   /* same as TK_FLOAT    */
#define OP_String8                             90   /* same as TK_STRING   */
#define OP_And                                 63   /* same as TK_AND      */
#define OP_BitNot                              89   /* same as TK_BITNOT   */
#define OP_NullRow                             20
#define OP_Noop                                21
#define OP_Ge                                  75   /* same as TK_GE       */
#define OP_HexBlob                            129   /* same as TK_BLOB     */
#define OP_ListRead                            22
#define OP_ParseSchema                         23
#define OP_Statement                           24
#define OP_CollSeq                             25
#define OP_ListWrite                           26
#define OP_ContextPop                          27
#define OP_MemIncr                             28
#define OP_MoveGe                              29
#define OP_Eq                                  71   /* same as TK_EQ       */
#define OP_If                                  30
#define OP_IfNot                               31
#define OP_ShiftRight                          80   /* same as TK_RSHIFT   */
#define OP_Destroy                             32
#define OP_Distinct                            33
#define OP_CreateIndex                         34
#define OP_SetNumColumns                       35
#define OP_Not                                 64   /* same as TK_NOT      */
#define OP_Gt                                  72   /* same as TK_GT       */
#define OP_ResetCount                          36
#define OP_Goto                                37
#define OP_IdxDelete                           38
#define OP_ListRewind                          39
#define OP_SortInsert                          40
#define OP_Found                               41
#define OP_MoveGt                              42
#define OP_MustBeInt                           43
#define OP_Prev                                44
#define OP_AutoCommit                          45
#define OP_String                              46
#define OP_Return                              47
#define OP_Callback                            48
#define OP_AddImm                              49
#define OP_Function                            50
#define OP_Concat                              86   /* same as TK_CONCAT   */
#define OP_NewRowid                            51
#define OP_AggContextPush                      52
#define OP_Blob                                53
#define OP_AggContextPop                       54
#define OP_IsNull                              68   /* same as TK_ISNULL   */
#define OP_Next                                55
#define OP_ForceInt                            56
#define OP_ReadCookie                          57
#define OP_Halt                                58
#define OP_SortNext                            59
#define OP_Expire                              60
#define OP_Or                                  62   /* same as TK_OR       */
#define OP_DropIndex                           61
#define OP_IdxInsert                           65
#define OP_ShiftLeft                           79   /* same as TK_LSHIFT   */
#define OP_AggNext                             66
#define OP_Column                              67
#define OP_Gosub                               76
#define OP_RowData                             88
#define OP_BitOr                               78   /* same as TK_BITOR    */
#define OP_MemMax                              91
#define OP_Close                               92
#define OP_VerifyCookie                        93
#define OP_AggReset                            94
#define OP_IfMemPos                            95
#define OP_Null                                96
#define OP_Integer                             97
#define OP_Transaction                         98
#define OP_Divide                              84   /* same as TK_SLASH    */
#define OP_IdxLT                               99
#define OP_Delete                             100
#define OP_AggInit                            101
#define OP_Rewind                             102
#define OP_Push                               103
#define OP_Clear                              104
#define OP_Vacuum                             105
#define OP_OpenTemp                           106
#define OP_IsUnique                           107
#define OP_OpenWrite                          108
#define OP_Negative                            87   /* same as TK_UMINUS   */
#define OP_Le                                  73   /* same as TK_LE       */
#define OP_AbsValue                           109
#define OP_Sort                               110
#define OP_NotFound                           111
#define OP_MoveLe                             112
#define OP_MakeRecord                         113
#define OP_Add                                 81   /* same as TK_PLUS     */
#define OP_SortReset                          114
#define OP_Variable                           115
#define OP_Ne                                  70   /* same as TK_NE       */
#define OP_AggFocus                           116
#define OP_CreateTable                        117
#define OP_Insert                             118
#define OP_AggGet                             119
#define OP_IdxGE                              120
#define OP_OpenRead                           121
#define OP_IdxRowid                           122
#define OP_Lt                                  74   /* same as TK_LT       */
#define OP_ListReset                          123
#define OP_Pop                                124

/* The following opcode values are never used */
#define OP_NotUsed_125                        125
#define OP_NotUsed_126                        126
#define OP_NotUsed_127                        127

#define NOPUSH_MASK_0 63098
#define NOPUSH_MASK_1 65463
#define NOPUSH_MASK_2 49146
#define NOPUSH_MASK_3 62931
#define NOPUSH_MASK_4 65527
#define NOPUSH_MASK_5 64191
#define NOPUSH_MASK_6 57340
#define NOPUSH_MASK_7 6997
#define NOPUSH_MASK_8 0
#define NOPUSH_MASK_9 0
