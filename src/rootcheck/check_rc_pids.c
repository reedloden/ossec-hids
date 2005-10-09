/*   $OSSEC, check_rc_pids.c, v0.1, 2005/10/05, Daniel B. Cid$   */

/* Copyright (C) 2005 Daniel B. Cid <dcid@ossec.net>
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
 
#include <stdio.h>       
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <errno.h>

#include "headers/defs.h"
#include "headers/debug_op.h"

#include "rootcheck.h"

/** Prototypes **/
void loop_all_pids(char *ps, pid_t max_pid, int *_errors, int *_total)
{
    int _kill0 = 0;
    int _kill1 = 0;
    int _gsid0 = 0;
    int _gsid1 = 0;
    int _ps0 = -1;
    
    pid_t i = 1;

    char command[OS_MAXSTR +1];
    
    for(;;i++)
    {
        if((i <= 0)||(i > max_pid))
            break;

        (*_total)++;
        
        _kill0 = 0;
        _gsid0 = 0;
        _gsid1 = 0;
        _ps0 = -1;
        
        if(!((kill(i, 0) == -1)&&(errno == ESRCH)))
        {
            _kill0 = 1;
        }
        
        if(!((getsid(i) == -1)&&(errno == ESRCH)))
        {
            _gsid0 = 1;
        }

        /* IF PID does not exist, keep going */
        if(!_kill0 && !_gsid0)
        {
            continue;
        }
        
        /* checking if process appears on ps */
        if(*ps)
        {
            snprintf(command, OS_MAXSTR, "%s -p %d > /dev/null 2>&1", ps, i);

            /* Found PID on ps */
            _ps0 = 0;
            if(system(command) == 0)
                _ps0 = 1;
        }
       
        /* If our kill or getsid system call, got the
         * PID , but ps didn't check if it was a problem
         * with a PID being deleted (not used anymore )
         */
        if(!_ps0)
        {
            if(!((getsid(i) == -1)&&(errno == ESRCH)))
            {
                _gsid1 = 1;
            }
            
            if(!((kill(i, 0) == -1)&&(errno == ESRCH)))
            {
                _kill1 = 1;
            }

            /* If it matches, process was terminated */
            if(_gsid0 && _kill1)
            {
                continue;
            }
        }
        
        if(_gsid0 != _kill0)
        {
            char op_msg[OS_MAXSTR +1];
        
            snprintf(op_msg, OS_MAXSTR, "Process '%d' hidden from "
                             "kill or getsid. Possible kernel-level "
                             "rootkit.", i);
            
            notify_rk(ALERT_ROOTKIT_FOUND, op_msg);
            (*_errors)++;
        }

        else if(_gsid0 && _kill0 && !_ps0)
        {
            char op_msg[OS_MAXSTR +1];
            snprintf(op_msg, OS_MAXSTR, "Process '%d' hidden from "
                             "ps. Possible trojaned version installed.",i);
           
            notify_rk(ALERT_ROOTKIT_FOUND, op_msg); 
            (*_errors)++;
        }
    }
}


/*  check_rc_sys: v0.1
 *  Scan the whole filesystem looking for possible issues
 */
void check_rc_pids()
{
    int _total = 0;
    int _errors = 0;
    
    char ps[OS_MAXSTR +1];
    pid_t max_pid;
    
    /* Default max pid for most systems */
    max_pid = 32768;


    /* Checking where ps is */
    strcpy(ps, "/bin/ps");
    if(!is_file(ps))
    {
        strcpy(ps, "/usr/bin/ps");
        if(!is_file(ps))
            ps[0] = '\0';
    }
    
    loop_all_pids(ps, max_pid, &_errors, &_total);

    if(_errors == 0)
    {
        char op_msg[OS_MAXSTR +1];
        snprintf(op_msg, OS_MAXSTR, "No hidden process by Kernel-level "
                                    "rootkits.\n      %s is not trojaned. "
                                    "Analized %d processes.", ps, _total);
        notify_rk(ALERT_OK, op_msg);
    }
    
    return;
}

/* EOF */