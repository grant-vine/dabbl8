// SPDX-License-Identifier: GPL-3.0-only
// Fixture scaffolding only; exact extracted MIT source remains separately credited.
#include <cstdint>
#include <string>
#include <cstring>
#include <cstdlib>
#include <new>
#include <cstdio>
using MIDIClientRef=uint32_t;using MIDIPortRef=uint32_t;using MIDIEndpointRef=uint32_t;
struct MIDISysexSendRequest { void *opaque[5]; };
static unsigned poisoned=0,valid=0;
void *operator new(std::size_t n){void*p=std::malloc(n);if(!p)throw std::bad_alloc();std::memset(p,0xa5,n);return p;}
void operator delete(void*p) noexcept {std::free(p);}
void operator delete(void*p,std::size_t) noexcept {std::free(p);}
void MIDIEndpointDispose(MIDIEndpointRef){std::abort();}
void MIDIPortDispose(MIDIPortRef p){if(p==0xa5a5a5a5u)++poisoned;else if(p==0x1234u)++valid;else std::abort();}
struct MidiInApi {void*apiData_=nullptr;bool connected_=false;struct {void*apiData;}inputData_;MidiInApi(unsigned){} };
struct MidiOutApi {void*apiData_=nullptr;bool connected_=false;};
class MidiInCore:public MidiInApi {public:MidiInCore(const std::string&,unsigned);~MidiInCore();void initialize(const std::string&);void closePort();MIDIClientRef getCoreMidiClientSingleton(const std::string&){return 77;} };
class MidiOutCore:public MidiOutApi {public:MidiOutCore(const std::string&);~MidiOutCore();void initialize(const std::string&);void closePort();MIDIClientRef getCoreMidiClientSingleton(const std::string&){return 77;} };
/**********************************************************************/
/*! \class RtMidi
    \brief An abstract base class for realtime MIDI input/output.

    This class implements some common functionality for the realtime
    MIDI input/output subclasses RtMidiIn and RtMidiOut.

    RtMidi GitHub site: https://github.com/thestk/rtmidi
    RtMidi WWW site: http://www.music.mcgill.ca/~gary/rtmidi/

    RtMidi: realtime MIDI i/o C++ classes
    Copyright (c) 2003-2021 Gary P. Scavone

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    Any person wishing to distribute modifications to the Software is
    asked to send the modifications to the original developer so that
    they can be incorporated into the canonical version.  This is,
    however, not a binding provision of this license.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/**********************************************************************/

struct CoreMidiData {
  MIDIClientRef client;
  MIDIPortRef port;
  MIDIEndpointRef endpoint;
  MIDIEndpointRef destinationId;
  unsigned long long lastTime;
  MIDISysexSendRequest sysexreq;
};

MidiInCore :: MidiInCore( const std::string &clientName, unsigned int queueSizeLimit )
  : MidiInApi( queueSizeLimit )
{
  MidiInCore::initialize( clientName );
}

MidiInCore :: ~MidiInCore( void )
{
  // Close a connection if it exists.
  MidiInCore::closePort();

  // Cleanup.
  CoreMidiData *data = static_cast<CoreMidiData *> (apiData_);
  if ( data->endpoint ) MIDIEndpointDispose( data->endpoint );
  delete data;
}

void MidiInCore :: initialize( const std::string& clientName )
{
  // Set up our client.
  MIDIClientRef client = getCoreMidiClientSingleton(clientName);

  // Save our api-specific connection information.
  CoreMidiData *data = (CoreMidiData *) new CoreMidiData;
  data->client = client;
  data->endpoint = 0;
  apiData_ = (void *) data;
  inputData_.apiData = (void *) data;
}

void MidiInCore :: closePort( void )
{
  CoreMidiData *data = static_cast<CoreMidiData *> (apiData_);

  if ( data->endpoint ) {
    MIDIEndpointDispose( data->endpoint );
    data->endpoint = 0;
  }

  if ( data->port ) {
    MIDIPortDispose( data->port );
    data->port = 0;
  }

  connected_ = false;
}

MidiOutCore :: MidiOutCore( const std::string &clientName )
  : MidiOutApi()
{
  MidiOutCore::initialize( clientName );
}

MidiOutCore :: ~MidiOutCore( void )
{
  // Close a connection if it exists.
  MidiOutCore::closePort();

  // Cleanup.
  CoreMidiData *data = static_cast<CoreMidiData *> (apiData_);
  if ( data->endpoint ) MIDIEndpointDispose( data->endpoint );
  delete data;
}

void MidiOutCore :: initialize( const std::string& clientName )
{
  // Set up our client.
  MIDIClientRef client = getCoreMidiClientSingleton(clientName);

  // Save our api-specific connection information.
  CoreMidiData *data = (CoreMidiData *) new CoreMidiData;
  data->client = client;
  data->endpoint = 0;
  apiData_ = (void *) data;
}

void MidiOutCore :: closePort( void )
{
  CoreMidiData *data = static_cast<CoreMidiData *> (apiData_);

  if ( data->endpoint ) {
    MIDIEndpointDispose( data->endpoint );
    data->endpoint = 0;
  }

  if ( data->port ) {
    MIDIPortDispose( data->port );
    data->port = 0;
  }

  connected_ = false;
}
int main(int argc,char**argv){const std::string name="mock"; {MidiInCore x(name,16);} {MidiOutCore x(name);} unsigned observed=poisoned;
 {MidiInCore x(name,16);static_cast<CoreMidiData*>(x.apiData_)->port=0x1234u;}
 {MidiOutCore x(name);static_cast<CoreMidiData*>(x.apiData_)->port=0x1234u;}
 printf("poisoned-dispose-attempts=%u valid-seeded-disposals=%u fake-client=77 no-OS-API\n",observed,valid);
 unsigned expected=argc>1?std::strtoul(argv[1],nullptr,10):0;
 if(observed!=expected||valid!=2){fprintf(stderr,"FAIL unexpected poisoned or valid disposal count\n");return 1;}puts("PASS actual extracted C++ lifecycle against fake CoreMIDI");}
