/*curl_util.cpp*/

//
// CURL utility functions for calling a web server.
// 
// Prof. Joe Hummel
// Northwestern University
// CS 211
// 
// References:
//
// CURL library for internet access:
//   https://everything.curl.dev/libcurl
//

#include <iostream>
#include <fstream>
#include <string>

#include "curl_util.h"

using namespace std;


//
// CURL_callback:
//
// This function is called when the CURL library receives a response 
// from the web site. The function appends the contents of the response
// into the output, returning the total # of characters copied.
//
static size_t CURL_callback(void* contents, size_t size, size_t nmemb, std::string* output) {

  size_t total_size = size * nmemb;

  output->append((char*)contents, total_size);

  return total_size;
}


//
// callWebServer:
//
// Given a URL, calls the web server attached to this URL and
// returns true if the web server responded, and false if not
// (false is also returned if there are problems with the CURL
// pointer that is passed). Note that the curl pointer should
// have been returned by a call to curl_easy_init().
//
// If true is returned, the response parameter will be set
// to the data returned by the server. If false is returned,
// response is unchanged.
//
// #define OFFLINE to test with offline saved data
// #define SAVE_ONLINE_RESPONSES if you want to save online data 
//    to use later when offline
//
static string getURLParam(string url, string key)
{
  auto pos = url.find(key);
  if (pos == string::npos)
    return "-1";
  
  auto endpos = url.find("&", pos);
  if (endpos == string::npos)
    return "-1";
  
  size_t start = pos + key.length();
  size_t len = endpos - start;
  return url.substr(start, len);
}

bool callWebServer(CURL* curl, string url, string& response)
{
  //
  // URL format:
  //
  // http://ctabustracker.com/bustime/api/v2/getpredictions?key=89dj2he89d8j3j3ksjhdue93j&rt=20&stpid=456&format=json
  //

#ifdef OFFLINE
  //
  // We are offline, so return data saved from online calls:
  //
  string route = getURLParam(url, "rt=");
  string stopid = getURLParam(url, "stpid=");
  
  string filename = "cta-response-route-";
  filename = filename + route + "-stopid-";
  filename = filename + stopid + ".cta";
 
  ifstream infile(filename);
  if (!infile.is_open()) {
    cout << "**ERROR: in callWebServer, we are running offline..." << endl;
    cout << "**ERROR: you called with rt=" << route << ",stpid=" << stopid << endl;
    cout << "**ERROR: no data is available for this route/stop" << endl;
    
    return false;
  }
  
  string line;
  response = "";
  
  while (getline(infile, line))
  {
    response += line;
    response += '\n';
  }
  
  infile.close();
  
  return true;

#else
  //
  // call the online web service:
  //
  curl_easy_reset(curl);

  int rc1 = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  int rc2 = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURL_callback);
  int rc3 = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  if (rc1 != CURLE_OK || rc2 != CURLE_OK || rc3 != CURLE_OK)
    return false;

  //
  // call web server, which triggers call to callback function when the
  // response arrives:
  //
  int rc = curl_easy_perform(curl);

  if (rc != CURLE_OK)
    return false;

  // 
  // if get here, it worked:
  //
#ifdef SAVE_ONLINE_RESPONSES
    string route = getURLParam(url, "rt=");
    string stopid = getURLParam(url, "stpid=");
   
    string filename = "cta-response-route-";
    filename = filename + route + "-stopid-";
    filename = filename + stopid + ".cta";
   
    ofstream outfile(filename);
    outfile << response << endl;
    outfile.close();
#endif
  
  return true;

#endif 
  
}

