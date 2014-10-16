#!/usr/sbin/dtrace -Zs

#pragma D option strsize=1024
#pragma D option quiet
#pragma D option bufsize=20m
#pragma D option switchrate=123hz

dtrace:::BEGIN
{
/*  printf("Tracing... Hit Ctrl-C to end.\n");*/
}

/* filename, classname, funcname */
javascript*:::function-entry
{
  printf("%d %s %s %s\n",timestamp,"entry",basename(copyinstr(arg0,1024)),copyinstr(arg2,1024));
/*  self->start = timestamp;
  @Counts[basename(copyinstr(arg0)), copyinstr(arg2)] = count();
*/}

/* filename, classname, funcname */
javascript*:::function-return
{
  printf("%d %s %s %s\n",timestamp,"return",basename(copyinstr(arg0,1024)),copyinstr(arg2,1024));
/*  this->elapsed = timestamp - self->start;
  @Elapsed[basename(copyinstr(arg0)), copyinstr(arg2)] = max(this->elapsed);
*/}

dtrace:::END
{
  /*
  printf("--COUNTS--\n");
  printf(" %-32s %-36s %8s\n", "FILE", "FUNC", "CALLS");
  printa(" %-32s %-36s %@8d\n", @Counts);

  printf("--MAX ELAPSED TIME--\n");
  printf(" %-32s %-36s %8s\n", "FILE", "FUNC", "TIME");
  printa(" %-32s %-36s %@8d\n", @Elapsed);
  */
}