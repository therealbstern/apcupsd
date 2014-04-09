/* Coverity models */

void error_out(const char *file, int line, const char *msg, ...)
{
   __coverity_panic__();
}
