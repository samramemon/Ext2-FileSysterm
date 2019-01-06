/************************************************************************
  Cpt_S 360: Lab 2 - Unix/Linux File System Tree Simulator

  Nofal Aamir
  WSU ID 11547300
************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

typedef struct node{
          char  name[64];       // node's name string
          char  type;
   struct node *child, *sibling, *parent;
}NODE;


NODE *root, *cwd, *start;
char line[128];                   // for getting user input line
char command[64], pathname[64];   // for command and pathname strings
char dname[64], bname[64];        // for dirname and basename 
char *name[100];                  // token string pointers
int  n;                           // number of token strings in pathname 
FILE *fp;
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
"save", "reload", "menu", "quit", NULL};

/************************************************************************
                       HELPERS
************************************************************************/
void initialize()
{
  //initializing root
  root = (NODE*)malloc(sizeof(NODE));
  root->type = 'D';
  root->child = NULL;
  root->sibling = root;
  root->parent = root;
  strcpy(root->name, "/");
  
  cwd = root;
  *name = (char*)malloc(100);
}

int dbname(char *pathname)
{
  char temp[128]; // dirname(), basename() destroy original pathname
  strcpy(temp, pathname);
  strcpy(dname, dirname(temp));
  strcpy(temp, pathname);
  strcpy(bname, basename(temp));
}

int findcmd(char *command)
{
  for(int i=0; cmd[i]!=NULL; i++)
    {
      if (strcmp(cmd[i], command) == 0)
	{
	  printf("%s\n", cmd[i]);
	  return i;
	}     
    }
  return -1;
}

// create a new node
NODE* newNode(char* name, char type) // create a new node
{
  NODE *t = (NODE *)malloc(sizeof(NODE));
  strcpy(t->name, name);
  t->type = type;
  t->child = t->sibling = NULL;
  return t;
}

// insert a new node with given key into BST
int insert(NODE *node, char *name, char type)
{
  NODE *p = node;
  NODE *c = node->child;
  
  if (p->child != NULL) {
    while (c->sibling != NULL) {
      c = c->sibling;
    }
    c->sibling = newNode(name, type);
    c->sibling->parent = c;
  }
  else {
    p->child = newNode(name, type);
    p->child->parent = p;
  }
  
}

NODE* search_child(NODE *parent, char *name)
{ 
  //Search for name under a parent node. 
  //Return the child node pointer if exists, 0 if not.
  NODE *p = parent;
  NODE *c = parent->child;

  if (p->child == NULL)
    return 0;
  
  if (strcmp(p->child->name, name) == 0)
    return p->child;
  else {
    while (c->sibling != NULL) {
      if (strcmp(c->sibling->name, name) == 0)
	return c->sibling;
      c = c->sibling;
    }
    return 0;
  }
}

int tokenize(char *pathname)
{
  char *s;
  char temp[100];
  int i = 0;
  
  strcpy(temp, pathname);  //make copy of pathname entered
  
  s = strtok(temp, "/");

  while (s){
    name[i] = s;   //store pointer to token in name[] array
    s = strtok(0, "/");
    i++;
  }
  return i;
}

NODE* path2node(char *pathname)
{
  //return the node pointer of a pathname, or 0 if the node does not exist. 
  if (pathname[0] == '/')
    start = root;
  else
    start = cwd;

  n = tokenize(pathname);
  
  NODE *p = start;

  for (int i=0; i<n; i++)
    {
      p = search_child(p, name[i]);
      if (p == 0)
	return 0;
    }
  return p;
}

int dir_empty(NODE *node)
{
  NODE *c = node->child;
  
  if (node->child == NULL)
    return 0;
  else
    return -1;
}

void delete(NODE *node)
{
  //assume last node and empty
  NODE *p = node;
  if(p->parent->child == p)
    p->parent->child = NULL;
  else if (p->parent->sibling == p)
    p->parent->sibling = NULL;

  free(node);
}

/************************************************************************
                       COMMANDS
************************************************************************/
int mkdir(char *pathname)
{
  printf("Command:%s\n", command);
  printf("*****my_mkdir******\n");

  if (pathname[0] == '\0'){
    printf("enter a pathname\n");
    return 0;
  }

  NODE *p;
  
  dbname(pathname);
  printf("dirname:%s   basename:%s\n", dname, bname);

  if (strcmp(dname, ".") == 0){
    p = root;
    insert(p, bname, 'D');
    return 0;
  }
  p = path2node(dname);
  if (p == NULL){
    printf("directory does not exist\n");
    return 0;
  }
  if (p->type == 'D')
    insert(p, bname, 'D');
  else
    printf("not a directory\n");
}

int rmdir(char *pathname)
{
  printf("Command:%s\n", command);
  printf("*****my_rmdir******\n");
  
  NODE *p;
  char tempath[64];

  if (pathname[0] == '\0'){
    printf("enter a pathname\n");
    return 0;
  }

  strcpy(tempath, pathname);
  p = path2node(tempath);
  if (p == NULL) {
    printf("node does not exist\n");
    return 0;
  }
  else if (p->type == 'F') {
    printf("Use rm to delete files\n");
    return 0;
  }
  else if (dir_empty(p) != 0) {
    printf("directory not empty\n");
    return 0;
  }
  else {
    delete(p);
  }
}

int ls()
{
  printf("Command:%s\n", command);
  printf("*****my_ls******\n");
  
  NODE *p = cwd;
  NODE *c = p->child;

  if (p->child == NULL) {
    printf("empty\n"); 
  }
  else {
    p = p->child;
    printf("Type %c  Name %s\n", p->type, p->name);
  
  
    while (c->sibling != NULL) {
      c = c->sibling;
      printf("Type %c  Name %s\n", c->type, c->name);
    }
  }
}

int cd(char *pathname)
{
  printf("Command:%s\n", command);
  printf("*****my_cd******\n");
  
  NODE *p;
  char tempath[64];

  if (pathname[0] == '\0'){
    printf("enter a pathname\n");
    return 0;
  }

  if (strcmp(pathname, "..") == 0) {
      cwd = cwd->parent;
      return 0;
  }
  strcpy(tempath, pathname);
  p = path2node(tempath);

  if (p == NULL)
    printf("path does not exist\n");
  else if (p->type == 'F')
    printf("dis a file!!\n");
  else
    cwd = p;
}

int pwd()
{
  //pwd = cwd-->parent;
  //name_pwd = cwd->name;
 
  printf("Command:%s\n", command);
  printf("*****my_pwd******\n");
  NODE *p = cwd;
  char *itr[64];
  int i = 1;
  int n;

  *itr = (char*)malloc(64);

  strcpy(itr[0], p->name);
  while (p->parent != root) {
    p = p->parent;
    itr[i] = p->name;
    i++;
  }
  n = i;
  for (n; n>1; n--) {
    printf("/%s", itr[n]);
  }
  printf("\n");
}

int creat(char *pathname)
{
  printf("Command:%s\n", command);
  printf("*****my_creat******\n");

  if (pathname[0] == '\0'){
    printf("enter a pathname\n");
    return 0;
  }
      
  NODE *p;
  
  dbname(pathname);
  printf("dirname:%s   basename:%s\n", dname, bname);

  if (strcmp(dname, ".") == 0){
    p = root;
    insert(p, bname, 'F');
    return 0;
  }
  p = path2node(dname);
  if (p == NULL){
    printf("directory does not exist\n");
    return 0;
  }
  if (p->type == 'D')
    insert(p, bname, 'F');
  else
    printf("not a directory\n");
}


int rm(char *pathname)
{
  printf("Command:%s\n", command);
  printf("*****my_rm******\n");
  NODE *p;
  char tempath[64];

  if (pathname[0] == '\0'){
    printf("enter a pathname\n");
    return 0;
  }

  strcpy(tempath, pathname);
  p = path2node(tempath);
  if (p == NULL) {
    printf("node does not exist\n");
    return 0;
  }
  else if (p->type == 'D') {
    printf("use rmdir to delete directories\n");
    return 0;
  }
  else {
    delete(p);
  }
}

int save()
{
  printf("Command:%s\n", command);
  printf("********save*****\n");
  fp = fopen("myfile", "w+"); // fopen a FILE stream for WRITE

  NODE *p = root;
  NODE *c = root->child;
  char *pathname[64];
  char types[64];
  int i = 1;

  *types = (char*)malloc(64);
  *pathname = (char*)malloc(64);
  fprintf(fp, "type:%c pathname:%s\n", p->type, p->name);
  pathname[0] = '/';

  while (c->child !=NULL) {
    
    while (c->sibling != NULL) {
      strcpy(pathname[i], c->name);
      c = c->sibling;
    }
  }
  fprintf(fp, "pathname:%s\n", pathname);
  fclose(fp);
}

int reload()
{
  printf("Command:%s\n", command);
}

int menu()
{
  printf("Command:%s\n", command);
  printf("Can do commands: mkdir,rmdir,ls,cd,pwd,creat,rm,save,reload,menu\n");
}


int main()
{
  int index; //command index
  int (*fptr[ ])(char *)={(int (*)())mkdir,rmdir,ls,cd,pwd,creat,rm,save,reload,menu};
  
  initialize(); //initialize root node of the file system tree
  
  while(1){
    printf("input a command line : ");
    fgets(line,128,stdin);
    line[strlen(line)-1] = 0;
    sscanf(line, "%s %s", command, pathname);
    printf("entered commands: %s %s\n", command, pathname);
    if (strcmp(command,"quit") == 0)
	break;
    
    index = findcmd(command);
    if (index >= 0) fptr[index](pathname);  //pathname is the parameter being sent
    else printf("Enter a valid command\n");
  }

}
