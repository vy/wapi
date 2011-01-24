#include <fstream>
#include <string>
#include <utility>
#include <map>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

using namespace std;


static pair<string, string>
hostapd_split_conf(const string line)
{
  size_t eqchar = line.find("=");
  return make_pair(
    line.substr(0, eqchar),
    line.substr((eqchar+1), line.length())
    );
}


static bool
hostapd_read_conf(const char *conffile, map<string, string>& conf)
{
  ifstream ifs(conffile);
  if (!ifs)
  {
    fprintf(
      stderr, "Couldn't open %s for reading: %s",
      conffile, strerror(errno));
    return false;
  }

  string line;
  while (getline(ifs, line))
  {
    pair<string,string> entry = hostapd_split_conf(line);
    conf[entry.first] = entry.second;
  }

  return true;
}


static bool
hostapd_write_conf(const char *conffile, const map<string, string> conf)
{
  ofstream ofs(conffile);
  if (!ofs)
  {
    fprintf(
      stderr, "Couldn't open %s for writing: %s",
      conffile, strerror(errno));
    return false;
  }
  
  for (map<string, string>::const_iterator it = conf.begin();
       it != conf.end();
       ++it)
    ofs << it->first << "=" << it->second << endl;
  return true;
}


static bool
hostapd_reload(const char *pidfile)
{
  ifstream ifs(pidfile);
  string line;
  int pid;
  return (
    getline(ifs, line) &&
    sscanf(line.c_str(), "%d", &pid) &&
    !kill(pid, SIGHUP));
}


int
main(int argc, char *argv[])
{
  if (argc != 5)
  {
    fprintf(stderr, "Usage: %s <PIDFILE> <CONFFILE> <VAR> <VAL>\n", argv[0]);
    return 1;
  }
  const char *pidfile = argv[1];
  const char *conffile = argv[2];
  const string var = string(argv[3]);
  const string val = string(argv[4]);

  map<string, string> conf;
  hostapd_read_conf(conffile, conf);
  conf[var] = val;
  hostapd_write_conf(conffile, conf);
  hostapd_reload(pidfile);

  return 0;
}
