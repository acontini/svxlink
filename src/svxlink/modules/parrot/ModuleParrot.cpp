/**
@file	 ModuleParrot.cpp
@brief   A module that implements a "parrot" function.
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This module implements a "parrot" function. It plays back everything you say
to it. This can be used as a simplex repeater or just so you can hear how
you sound.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004  Tobias Blomberg

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <version/MODULE_PARROT.h>

#include <AsyncConfig.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleParrot.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define DEFAULT_MODULE_TIMEOUT	"300"


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Pure C-functions
 *
 ****************************************************************************/


extern "C" {
  Module *module_init(void *dl_handle, Logic *logic, int id,
      	      	      const char *cfg_name)
  {
    return new ModuleParrot(dl_handle, logic, id, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleParrot::ModuleParrot(void *dl_handle, Logic *logic, int id,
      	      	      	   const string& cfg_name)
  : Module(dl_handle, logic, id), fifo(30*8000), squelch_is_open(false),
    module_tmo_timer(0)
{
  cout << "\tModule " << name()
       << " v" MODULE_PARROT_VERSION " starting...\n";
  
  fifo.stopOutput(true);
  fifo.writeSamples.connect(slot(this, &ModuleParrot::audioFromFifo));
  fifo.allSamplesWritten.connect(slot(this, &ModuleParrot::allSamplesWritten));
  
  string timeout;
  if (!cfg().getValue(cfg_name, "TIMEOUT", timeout))
  {
    timeout = DEFAULT_MODULE_TIMEOUT;
  }
  module_tmo_timer = new Timer(1000 * atoi(timeout.c_str()));
  module_tmo_timer->setEnable(false);
  module_tmo_timer->expired.connect(slot(this, &ModuleParrot::moduleTimeout));
  
} /* ModuleParrot */


ModuleParrot::~ModuleParrot(void)
{
  delete module_tmo_timer;
} /* ~ModuleParrot */





/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    activateInit
 * Purpose:   Called by the core system when this module is activated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleParrot::activateInit(void)
{
  fifo.clear();
  module_tmo_timer->setEnable(true);
} /* activateInit */


/*
 *----------------------------------------------------------------------------
 * Method:    deactivateCleanup
 * Purpose:   Called by the core system when this module is deactivated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   Do NOT call this function directly unless you really know what
 *    	      you are doing. Use Module::deactivate() instead.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleParrot::deactivateCleanup(void)
{
  fifo.clear();
  module_tmo_timer->setEnable(false);
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received.
 * Input:     digit - The DTMF digit received (0-9, A-D, *, #)
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleParrot::dtmfDigitReceived(char digit)
{
  printf("DTMF digit received in module %s: %c\n", name(), digit);
  
} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceived
 * Purpose:   Called by the core system when a DTMF command has been
 *    	      received. A DTMF command consists of a string of digits ended
 *    	      with a number sign (#). The number sign is not included in the
 *    	      command string.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleParrot::dtmfCmdReceived(const string& cmd)
{
  printf("DTMF command received in module %s: %s\n", name(), cmd.c_str());
  
  if (cmd == "")
  {
    deactivateMe();
  }
  else
  {
    if (cmd == "0")
    {
      playHelpMsg();
    }
    else
    {
      playNumber(atoi(cmd.c_str()));
    }
  }
} /* dtmfCmdReceived */


void ModuleParrot::squelchOpen(bool is_open)
{
  squelch_is_open = is_open;
  
  if (is_open)
  {
    module_tmo_timer->setEnable(false);
    fifo.stopOutput(true);
  }
  else
  {
    if (!fifo.empty())
    {
      transmit(true);
      fifo.stopOutput(false);
    }
    module_tmo_timer->setEnable(true);
  }

} /* ModuleParrot::squelchOpen */


int ModuleParrot::audioFromRx(short *samples, int count)
{
  if (squelch_is_open)
  {
    //printf("Adding samples to FIFO...\n");
    fifo.addSamples(samples, count);
  }
  
  return count;
  
} /* ModuleParrot::audioFromRx */


int ModuleParrot::audioFromFifo(short *samples, int count)
{
  //printf("Writing %d samples from FIFO...\n", count);
  audioFromModule(const_cast<short *>(samples), count);
  return count;
} /* ModuleParrot::audioFromFifo */


void ModuleParrot::allSamplesWritten(void)
{
  transmit(false);
  fifo.stopOutput(true);
} /* ModuleParrot::allSamplesWritten */


void ModuleParrot::moduleTimeout(Timer *t)
{
  cout << "Module timeout: " << name() << endl;
  //playMsg("module");
  //playModuleName();
  playMsg("timeout");
  deactivateMe();
} /* ModuleParrot::moduleTimeout */



/*
 * This file has not been truncated
 */
