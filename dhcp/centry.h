#ifndef CENTRY_H_
#define CENTRY_H_

#define STAT_WAIT_DISCOVER 0
#define STAT_WAIT_REQUEST 1
#define STAT_IP_ASSIGNMENT 2

//Data structure
struct c_entry{
  //Bi-directional pointers

  //client ip, portnum, lease time, server status

};
//extern functions
extern struct c_entry* make_new_client();
extern struct c_entry* search_client();
extern int rm_client(int ip);

#endif // CENTRY_H_
