#ifndef EMBED_PERL_H_
#define EMBED_PERL_H_

#include <string>
extern "C" {
#define PERLIO_NOT_STDIO 0
#define USE_PERLIO
#include <EXTERN.h>
#include <perl.h>
}

#ifdef New
#undef New
#endif

EXTERN_C void xs_init(pTHX);

class EmbedPerl {
 public:

    EmbedPerl() {
	my_perl = perl_alloc();
	perl_construct(my_perl);
    }

    ~EmbedPerl() {
	PL_perl_destruct_level = 2;
	perl_destruct(my_perl);
	perl_free(my_perl);
    }

    int run(int argc, const std::string *argv, std::string *out,
	    std::string *err) {

	int exitstatus = 0;

	PERL_SYS_INIT3(&argc, (char ***) &argv, (char ***) NULL);

	PL_perl_destruct_level = 2;
	exitstatus = perl_parse(my_perl, xs_init, argc, (char **) argv,
				(char **) NULL);
	if (exitstatus != 0) {
	    return exitstatus;
	}

	ENTER;SAVETMPS;

	SV *outsv = sv_newmortal();
	SV *errsv = sv_newmortal();

	this->override_stdhandle(my_perl, outsv, "STDOUT");
	this->override_stdhandle(my_perl, errsv, "STDERR");

	perl_run(my_perl);

	this->restore_stdhandle(my_perl, "STDOUT");
	this->restore_stdhandle(my_perl, "STDERR");

	STRLEN outlen = SvCUR(outsv);
	char *tmpout = SvPV_nolen(outsv);
	*out = tmpout;

	STRLEN errlen = SvCUR(errsv);
	char * tmperr = SvPV_nolen(errsv);
	*err = tmperr;

	FREETMPS;LEAVE;

	PERL_SYS_TERM();

	return 0;
    }

 private:

    PerlInterpreter *my_perl;

    void override_stdhandle (pTHX_ SV *sv,const char *name ) {
	int status;
	GV *handle = gv_fetchpv(name,TRUE,SVt_PVIO);
	SV *svref = newRV_inc(sv);

	save_gp(handle, 1);

	status = Perl_do_open9(aTHX_ handle, ">:scalar", 8 , FALSE, O_WRONLY, 0, Nullfp, svref, 1);
	if(status == 0) {
	    Perl_croak(aTHX_ "Failed to open %s: %" SVf,name, get_sv("!",TRUE));
	}
    }

    void restore_stdhandle (pTHX_ const char *name) {
	int status;
	GV *handle = gv_fetchpv(name,FALSE,SVt_PVIO);

	if( GvIOn(handle) && IoOFP(GvIOn(handle)) && (PerlIO_flush(IoOFP(GvIOn(handle))) == -1 ) ) {
	    Perl_croak(aTHX_ "Failed to flush %s: " SVf,name,get_sv("!",TRUE) );
	}
    }

};

#endif /* EMBED_PERL_H_ */
