#include "lib/mlrutil.h"
#include "containers/lrec.h"
#include "containers/mlrval.h"
#include "containers/slls.h"
#include "containers/mixutil.h"
#include "mapping/mappers.h"

typedef struct _mapper_sec2gmt_state_t {
	slls_t*  pfield_names;
} mapper_sec2gmt_state_t;

static sllv_t*   mapper_sec2gmt_process(lrec_t* pinrec, context_t* pctx, void* pvstate);
static void      mapper_sec2gmt_free(mapper_t* pmapper);
static mapper_t* mapper_sec2gmt_alloc(slls_t* pfield_names);
static void      mapper_sec2gmt_usage(FILE* o, char* argv0, char* verb);
static mapper_t* mapper_sec2gmt_parse_cli(int* pargi, int argc, char** argv);

// ----------------------------------------------------------------
mapper_setup_t mapper_sec2gmt_setup = {
	.verb = "sec2gmt",
	.pusage_func = mapper_sec2gmt_usage,
	.pparse_func = mapper_sec2gmt_parse_cli,
};

// ----------------------------------------------------------------
static void mapper_sec2gmt_usage(FILE* o, char* argv0, char* verb) {
	fprintf(o, "Usage: %s %s {comma-separated list of field names}\n", argv0, verb);
	fprintf(o, "Replaces a numeric field representing seconds since the epoch with the\n");
	fprintf(o, "corresponding GMT timestamp. This is nothing more than a keystroke-saver for\n");
	fprintf(o, "the sec2gmt function:\n");
	fprintf(o, "  %s %s time1,time2\n", argv0, verb);
	fprintf(o, "is the same as\n");
	fprintf(o, "  %s put '$time1=sec2gmt($time1);$time2=sec2gmt($time2)'\n", argv0);
}

// ----------------------------------------------------------------
static mapper_t* mapper_sec2gmt_parse_cli(int* pargi, int argc, char** argv) {
	if ((argc - *pargi) < 2) {
		mapper_sec2gmt_usage(stderr, argv[0], argv[*pargi]);
		return NULL;
	}
	// verb:
	(*pargi)++;
	// field names:
	char* field_names_string = argv[(*pargi)++];
	slls_t* pfield_names = slls_from_line(field_names_string, ',', FALSE);

	return mapper_sec2gmt_alloc(pfield_names);
}

// ----------------------------------------------------------------
static mapper_t* mapper_sec2gmt_alloc(slls_t* pfield_names)
{
	mapper_t* pmapper = mlr_malloc_or_die(sizeof(mapper_t));

	mapper_sec2gmt_state_t* pstate = mlr_malloc_or_die(sizeof(mapper_sec2gmt_state_t));
	pstate->pfield_names = pfield_names;
	pmapper->pprocess_func = mapper_sec2gmt_process;
	pmapper->pvstate       = (void*)pstate;
	pmapper->pfree_func    = mapper_sec2gmt_free;

	return pmapper;
}

static void mapper_sec2gmt_free(mapper_t* pmapper) {
	mapper_sec2gmt_state_t* pstate = pmapper->pvstate;
	slls_free(pstate->pfield_names);
	free(pstate);
	free(pmapper);
}

// ----------------------------------------------------------------
static sllv_t* mapper_sec2gmt_process(lrec_t* pinrec, context_t* pctx, void* pvstate) {
	if (pinrec == NULL) // end of stream
		return sllv_single(NULL);

	mapper_sec2gmt_state_t* pstate = (mapper_sec2gmt_state_t*)pvstate;

	for (sllse_t* pe = pstate->pfield_names->phead; pe != NULL; pe = pe->pnext) {
		char* name = pe->value;
		char* sval = lrec_get(pinrec, name);
		if (sval == NULL)
			continue;

		if (*sval == 0) {
			lrec_put(pinrec, name, "", NO_FREE);
		} else {
			mv_t mval = mv_scan_number_or_die(sval);
			mv_t stamp = time_string_from_seconds(&mval, ISO8601_TIME_FORMAT);

			lrec_put(pinrec, name, stamp.u.strv, FREE_ENTRY_VALUE);
		}
	}
	return sllv_single(pinrec);
}
