/*
 Name             : TwitchCBotIRC.h
 Author           : Clement Campagna
 Version          : 0.1
 Copyright        : Under MIT (X11) License - Copyright 2017 Clement Campagna
 Description      : TwitchCBot is a simple C++(11) bot for Twitch which provides its users with a source code
                    well commented and fully functioning, readily open to improvements and alterations.
 Requirements     : This program was mainly developed using C++ but also makes reference to some C++11 instructions.
 Usage            : TwitchCBot <your_username> <OAuth_token> <channel>
 Tested in        : Linux Mint 18.1
 Author's Website : https://clementcampagna.net

 Licensed under the MIT License, Version X11 (the "License"); you may not use this file except
 in compliance with the License. You may obtain a copy of the License at

 https://opensource.org/licenses/MIT

 Unless required by applicable law or agreed to in writing, software distributed under the
 License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 either express or implied.  See the License for the specific language governing permissions and
 limitations under the License.
*/

#ifndef TWITCHCBOTIRC_H_
#define TWITCHCBOTIRC_H_

#include <string>
#include <chrono>

using namespace std;

class TwitchCBotIRC
{
public:
	TwitchCBotIRC(string, string, string);
    virtual ~TwitchCBotIRC();
    void start();

private:
    int commSocket;
    int sentDataCounter;

    string nick;
    string pass;
    string channel;
    string speak;
    string uptime;

    chrono::system_clock systemClock;
    chrono::system_clock::time_point lastRun;

    char *timeNow();
    void sendData(string);
    void sendPong(char*);
    string formatTwitchUsernameOrMessage(string, bool);
    string toLower(string);
    void msgHandler(char*);
};

#endif /* TWITCHCBOTIRC_H_ */
