#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <stack>
#include <utility>
#include <string>
#include <sstream>


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


using namespace std;

#define TYPE_MAX_SIZE 32
#define SOURCE_MAX_SIZE 64
#define FUNCNAME_MAX_SIZE 512

enum TYPE
{
  ENTRY, RETURN
};

struct LOG
{
  TYPE            type;
  long long int   time;
  char            source[SOURCE_MAX_SIZE];
  char            funcname[FUNCNAME_MAX_SIZE];
};

vector<LOG*>  logs;

enum MODE {INCLUDE, EXCLUDE};

struct OPTS
{
  MODE mode;
  int  filter_above;
  int top;
} opts;



bool read(const char * filepath)
{
  FILE * fp = fopen(filepath, "r");
  if (!fp) return false;
  char buf[1024];
  char typebuf[TYPE_MAX_SIZE];

  while (fgets(buf, 1024, fp))
  {
    LOG * l = new LOG();
    if (buf[0] == '\n') continue;

    sscanf(buf, "%lld %s %s %s", &l->time, typebuf, l->source, l->funcname);
    if (strcmp(typebuf, "entry") == 0)
      l->type = ENTRY;
    else if (strcmp(typebuf, "return") == 0)
      l->type = RETURN;
    else
    {
      fprintf(stderr, "Unknown type %s\n", typebuf);
      delete l;
      continue;
    }
    logs.push_back(l);
  }

  fclose(fp);
  return true;
}

struct NODE
{
  vector<NODE*> children;
  long long int elapsed_time;
  char          source[SOURCE_MAX_SIZE];
  char          funcname[FUNCNAME_MAX_SIZE];

  NODE(const char * source_, const char * funcname_)
  : children(), elapsed_time(), source(), funcname()
  {
    strcpy(source, source_);
    strcpy(funcname, funcname_);
  }
};

vector<NODE*> roots;
vector<pair<int,NODE*>> tops;

void dump_node(int index, NODE * node)
{
  double time_ms = node->elapsed_time/1000000.0;
  if (opts.filter_above == -1 || time_ms > opts.filter_above)
    printf("[%3d] %11.3lf(ms) : %s %s\n", index, time_ms, node->source, node->funcname);
}

void write(const char * nodepath)
{
  struct STACK_ELEMENT {long long int begintime; NODE * node; int line;};
  stack<STACK_ELEMENT> nodestack;

  for (int i = 0; i < logs.size(); i++)
  {
    auto l = logs[i];
    // if (l->type == RETURN)
    //   printf("%s %s\n", l->source, l->funcname);
    if (l->type == ENTRY)
    {
      STACK_ELEMENT el;
      el.begintime = l->time;
      el.node = new NODE(l->source, l->funcname);
      el.line = i;
      nodestack.push(el);
    }
    else if (l->type == RETURN)
    {
      if (nodestack.empty())
      {
        NODE * root = new NODE(l->source, l->funcname);
        long long int sum = 0;
        for (auto & node : roots)
        {
          sum += node->elapsed_time;
        }
        root->elapsed_time = sum;
        root->children = roots;
        roots.clear();
        roots.push_back(root);
      }
      else
      {
        STACK_ELEMENT self = nodestack.top();
        nodestack.pop();

        if (strcmp(self.node->source, l->source) != 0 || strcmp(self.node->funcname, l->funcname) != 0)
        {
          fprintf(stderr, "Pair dosen't matched. [current:%d]%s %s [stack:%d]%s %s\n", i, l->source, l->funcname, self.line, self.node->source, self.node->funcname);
          self.node->elapsed_time = 0;
          for (auto child : self.node->children)
            self.node->elapsed_time += child->elapsed_time;

          if (nodestack.empty())
            roots.push_back(self.node);
          else
          {
            nodestack.top().node->children.push_back(self.node);
            NODE * prev_top = nodestack.top().node;
            do
            {
              nodestack.top().node->children.push_back(prev_top);
              prev_top = nodestack.top().node;
              nodestack.pop();
            } while (!nodestack.empty());
            roots.push_back(prev_top);
          }
          continue;
        }

        self.node->elapsed_time = l->time - self.begintime;
        if (opts.top > 0 && self.node->children.size() == 0)
        {
          int count = 0;
          for (auto it = tops.begin(); it != tops.end(); it++, count++)
          {
            if ((*it).second->elapsed_time < self.node->elapsed_time)
              break;
          }
          tops.insert(tops.begin() + count, pair<int,NODE*>(self.line, self.node));
          if (tops.size() > opts.top)
            tops.resize(opts.top);
        }

        if (nodestack.empty())
          roots.push_back(self.node);
        else
          nodestack.top().node->children.push_back(self.node);
      }
    }
  }


  vector<int> nodepath_by_index;
  auto splited = split(nodepath, '/');
  for (auto & num : splited)
  {
    if (num.length() > 0)
      nodepath_by_index.push_back(atoi(num.c_str()));
  }

  vector<NODE *> * cwd = &roots;
  for (auto index : nodepath_by_index)
  {
    if (cwd->size() <= index)
    {
      fputs("Invalid nodepath", stderr);
      return;
    }
    cwd = &(*cwd)[index]->children;
  }
  printf("--- %lu\n", cwd->size());
  int count = 0;
  for (auto node : *cwd)
  {
    dump_node(count++, node);
  }

  if (opts.top > 0)
  {
    printf("--- TOPS:%d\n", opts.top);
    int count = 0;
    for (auto pair : tops)
    {
      dump_node(pair.first, pair.second);
    }
  }
}


void dealloc_node(NODE * self)
{
  for (auto child : self->children)
    dealloc_node(child);

  delete self;
}

void dealloc()
{
  for (auto l : logs) delete l;
  for (auto root : roots) dealloc_node(root);
}

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    fputs("Usage : test123 [filepath] [nodepath]", stderr);
    exit(1);
  }

  const char * filepath = argv[1];
  if (!read(filepath))
  {
    fputs("Read failed.", stderr);
    exit(1);
  }

  const char * nodepath = argv[2];

  opts.filter_above = -1;
  opts.mode = EXCLUDE;
  opts.top = -1;
  for (int i = 3; i < argc; i++)
  {
    if (strcmp(argv[i], "--above") == 0)
      opts.filter_above = atoi(argv[++i]);
    else if (strcmp(argv[i], "--excl") == 0)
      opts.mode = EXCLUDE;
    else if (strcmp(argv[i], "--incl") == 0)
      opts.mode = INCLUDE;
    else if (strcmp(argv[i], "--top") == 0)
      opts.top = atoi(argv[++i]);
  }

  write(nodepath);

  // dealloc();

  return 0;
}