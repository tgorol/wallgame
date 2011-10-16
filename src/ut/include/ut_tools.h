#ifndef _UT_TOOLS_H
#define _UT_TOOLS_H

typedef struct Ut_stats{
    wg_status status;
    wg_uint passed;
    wg_uint failed;
}Ut_stats;

#define STRINGIFY(str) #str

#define UT_DEFINE(name)                                                   \
    void name(Ut_stats *stats){                                           \
        stats->status = WG_SUCCESS;


#define UT_END                                                            \
    return; }

#define UT_PASS_ON(cond)                                                  \
    if (cond){                                                            \
        fprintf(stdout, "PASS:line %d --> %s\n", __LINE__,                \
                STRINGIFY(cond));                                         \
        ++stats->passed;                                                  \
    }else{                                                                \
        fprintf(stdout, "FAIL:line %d --> %s\n", __LINE__,                \
                STRINGIFY(cond));                                         \
        ++stats->failed;                                                  \
        stats->status = WG_FAILURE;                                       \
    }

#define UT_RUN_TEST(test)                                                 \
    do{                                                                   \
        Ut_stats stats;                                                   \
        stats.status = WG_SUCCESS;                                        \
        stats.failed = 0;                                                 \
        stats.passed = 0;                                                 \
        printf("Starting test: %s\n", STRINGIFY(test));                   \
        test(&stats);                                                     \
        printf("Results:\n"                                               \
               "Passed Tests : %d\n"                                      \
               "Failed Tests : %d\n"                                      \
               "Test %s %s\n",                                            \
                stats.passed, stats.failed,                               \
                STRINGIFY(test), (stats.status == WG_SUCCESS              \
                    ? "SUCCESS"                                           \
                    : "FAILURE"));                                        \
    }while (0)


#endif
