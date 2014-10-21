/* Coverity models */

void error_out_wrapper(const char *file, int line, const char *msg, ...)
{
   __coverity_panic__();
}
