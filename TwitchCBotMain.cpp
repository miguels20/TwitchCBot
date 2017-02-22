/*
 Name             : TwitchCBotMain.cpp
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

#include <iostream>
#include <string>

#include "TwitchCBotIRC.h"

using namespace std;

/*
 * Split a string after a specified delimiter (last occurrence found)
 */
string SplitStrAfterDelimiter(const string &s)
{
    string::size_type pos = s.find_last_of('/'); /* use \\ for Windows systems */
    if (pos != string::npos)
    {
        return s.substr(pos + 1, s.size());
    }
    else
    {
        return s;
    }
}

int main(int argc, char **argv) {
	string app_name = SplitStrAfterDelimiter(argv[0]);

	if(argc != 4)
	{
		cout << "Hi, I am TwitchCBot, a simple Twitch robot created by Clement Campagna!" << endl;
		cout << "I'm here to help you :)" << endl << endl;
		cout << "You can invoke me by using the following command: " << app_name << " <your_username> <OAuth_token> <channel>" << endl;
		cout << "e.g.: " << app_name << " robin oauth:012345678901234567890123456789 xbox" << endl << endl;
		cout << "Please visit http://twitchapps.com/tmi/ to obtain your Twitch OAuth token." << endl << endl;
		cout << "See you next time!" << endl;
	}
	else
	{
		string nick = argv[1];
		string pass = argv[2];
		string channel = argv[3];

		TwitchCBotIRC bot = TwitchCBotIRC(nick, pass, channel);
		bot.start();
	}

	return 0;
}
