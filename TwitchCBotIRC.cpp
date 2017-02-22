/*
 Name             : TwitchCBotIRC.cpp
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

#include "TwitchCBotIRC.h"

#include <iostream>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;

#define MAX_DATA_SIZE 4096
#define SEND_MSG_OR_CMD_LIMIT_WITHIN_30_SEC 20

static const string SERVER = "irc.chat.twitch.tv";
static const string PORT = "6667";

struct to_lower
{
	int operator() (int ch)
	{
		return tolower (ch);
	}
};

TwitchCBotIRC::TwitchCBotIRC(string _nick, string _pass, string _channel)
{
	cout << "Hi, I am TwitchCBot, a simple Twitch robot created by Clement Campagna!" << endl;
	cout << "You are currently running version: 0.1 [Feb 22, 2017]" << endl;
	cout << "Please visit https://github.com/clementcampagna/TwitchCBot for updates" << endl << endl;
	cout << "-> Initializing TwitchCBot" << endl;

	//Initialize private variables
	commSocket = 0;
	sentDataCounter = 0;

    nick = _nick;
    pass = _pass;
    channel = "#" + toLower(_channel);
    speak = "PRIVMSG " + channel + " :";
}

TwitchCBotIRC::~TwitchCBotIRC()
{
    close(commSocket);
}

void TwitchCBotIRC::start()
{
    struct addrinfo flags, *serverInfo;

	//Ensure that addrinfo struct is clear
    memset(&flags, 0, sizeof flags);

    //Setup some connection flags
    flags.ai_family = AF_UNSPEC; //accept IPv4 or IPv6
    flags.ai_socktype = SOCK_STREAM; //TCP stream socket

    //Resolve SERVER's name to socket address
    cout << "-> Resolving server's IP address for [" + SERVER + "]" << endl;

    int res;
    if ((res = getaddrinfo(SERVER.c_str(), PORT.c_str(), &flags, &serverInfo)) != 0) /* getaddrinfo() returns 0 if it succeeds */
    {
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(res));
        abort();
    }

    //Setup the communication socket
    cout << "-> Setting up communication socket" << endl;

    if ((commSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) == -1)
    {
    	close(commSocket);
        perror("-- Socket error.");
        abort();
    }

    //Connect to Twitch IRC server
    cout << "-> Connecting to server" << endl;

    if (connect(commSocket,serverInfo->ai_addr, serverInfo->ai_addrlen) != 0) /* connect() returns 0 if it succeeds */
    {
        close(commSocket);
        perror("-- TwitchCBot Disconnected.");
        abort();
    }
	isConnected = true;

    //Free up some memory as we are now connected to the server, no need for serverInfo data anymore
    freeaddrinfo(serverInfo);

    //Set up our chat listener
    int readBytesCount;
    char readBuffer[MAX_DATA_SIZE];
    uptime = timeNow();

    //Retrieve actual time as it will be used for the 30sec countdown check implemented in sendData()
    lastRun = systemClock.now();

    bool setUpComplete = false;
	
    while (isConnected) {

    	if (!setUpComplete)
    	{
			//After connecting, immediately send pass, nick, and enable Twitch-specific capabilities (tags)
			sendData("PASS " + toLower(pass));
			sendData("NICK " + toLower(nick));
			sendData("CAP REQ :twitch.tv/tags");

			//Join channel passed as argument when launching the bot
			sendData("JOIN " + channel);

			//Send a Hello message to the channel
			sendData(speak + "Hi, my name is " + nick + "! I am a simple Twitch robot. Type !help for a list of commands I understand :-)");

			//No need to repeat this setup more than once
			setUpComplete = true;
    	}

        //Receive, format, and print IRC data. readBuffer contains the data that is received from the server
    	readBytesCount = recv(commSocket, readBuffer, MAX_DATA_SIZE -1, 0);
    	readBuffer[readBytesCount] = '\0';
    	string formatMsgOwner = formatTwitchUsernameOrMessage(string(readBuffer), true);
    	string formatMsgOrCmd = formatTwitchUsernameOrMessage(string(readBuffer), false);

    	//If data received was sent by a user (in chat), show it formatted
    	if ((formatMsgOwner.length() && formatMsgOrCmd.length()) > 0)
    		cout << "<- " << formatMsgOwner << " says: " << formatMsgOrCmd;
    	else
    		//Otherwise, display it raw
    		cout << "<- " << readBuffer;

        //Analyze each incoming message looking for a keyword we know (e.g. !bot, !help or !uptime)
        msgHandler(readBuffer);

        /* When a Ping request is received (every 5 minutes),
           we  must reply to it, otherwise our connection will be force-closed.
           See https://dev.twitch.tv/docs/v5/guides/irc/ for details */
        if (!strncmp(readBuffer, "PING :", 6)) //Ping received.
        	sendPong(readBuffer);

        //Break out of the while-loop if the connection to the server is lost
        if (readBytesCount <= 0)
        {	
        	cout << "-- " + SERVER + " closed the connection."<< endl;
        	cout << timeNow() << endl;
			isConnected = false;
        }
    }
}

char *TwitchCBotIRC::timeNow()
{
	//Returns the current date and time
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime (&rawtime);

    return asctime(timeinfo);
}

void TwitchCBotIRC::sendData(string msg)
{
	msg = msg.append("\r\n");
    int len = msg.length();

    //Reset sendDataCounter variable every 30 seconds
    if (systemClock.now() - lastRun >= chrono::seconds(30))
    {
    	lastRun += chrono::seconds(30);
    	sentDataCounter = 0;
    }

    /* As per Twitch guidelines, if you send more than 20 commands or messages
     * to the server within 30 seconds, you will be locked out for 30 minutes.
     * Note that if you send commands/messages only to channels in which you have
     * Moderator or Operator status, the limit is 100 messages per 30 seconds. */

    //This if-statement makes sure that we do not go over the limit
    if (sentDataCounter < SEND_MSG_OR_CMD_LIMIT_WITHIN_30_SEC)
    {
    	send(commSocket, msg.c_str(), len, 0);
    	sentDataCounter++;
    }

    cout << "-> " << msg;
}

void TwitchCBotIRC::sendPong(char *buf)
{
	string buffer = string(buf);
	if (buffer.length() > 6)
	{
		if (!strncmp(buf, "PING :", 6))
		{
			buffer.erase(0, 6);
			int found = buffer.find_first_of('\n', 0);

			if ((unsigned)found != string::npos)
				buffer.erase(found, buffer.length());

			sendData("PONG :" + buffer);
		}
	}
}

string TwitchCBotIRC::formatTwitchUsernameOrMessage(string msg, bool isUsername)
{
	if (msg.length() > 0)
	{
		if (isUsername)
		//Format username
		{
			int startIndex = msg.rfind("display-name=");
			msg.erase(0, startIndex + 13);
			int stopIndex = msg.find_first_of(';');

			if (stopIndex > 0)
			{
				msg.erase(stopIndex, msg.length());

				return msg;
			}
			else
			//Twitch user does not have a display name
			{
				int startIndex = msg.rfind("user-type=");
				msg.erase(0, startIndex + 10);
				int stopIndex = msg.find_first_of('!');

				if (stopIndex > 0)
				{
					msg.erase(stopIndex, msg.length());

					return msg;
				}
				else
					return "";
			}
		}
		else
		//Format message
		{
			int startIndex = msg.rfind(channel + " :");

			//We only want to format a user message or command, so msg must contain a user-type
			int isUserMessageOrCmd = msg.rfind("user-type=");
			if (isUserMessageOrCmd != -1)
			{
				if (startIndex > 0)
				{
					msg.erase(0, startIndex + (channel.length() + 2));

					return msg;
				}
				else
					return " ";
			}
			else
				return "";
		}
	}

	return "";
}

string TwitchCBotIRC::toLower(string text)
{
	transform(text.begin(), text.end(), text.begin(), to_lower());

	return text;
}

void TwitchCBotIRC::msgHandler(char *buf)
{
	string formatMsgOwner = formatTwitchUsernameOrMessage(string(buf), true);
	string formatMsgOrCmd = toLower(formatTwitchUsernameOrMessage(string(buf), false));

	if (!formatMsgOrCmd.empty())
	{
		if (!strncmp(formatMsgOrCmd.c_str(), "!hi", 3))
		{
			sendData(speak + "@" + formatMsgOwner + " Hi, my name is " + nick + "! I am a simple Twitch robot. Type !help for a list of commands I understand :-)");
		}
		else if (!strncmp(formatMsgOrCmd.c_str(), "!help", 5))
		{
			sendData(speak + "@" + formatMsgOwner + " Well, it seems that right now, the only three commands I know are !hi, !help and !uptime");
		}
		else if (!strncmp(formatMsgOrCmd.c_str(), "!uptime", 7))
		{
			sendData(speak + "@" + formatMsgOwner + " I've been up since " + uptime);
		}
		else if (!strncmp(formatMsgOrCmd.c_str(), "!anything_else", 14))
		{
			//Do something else
		}
		else /* default: */
		{
			//do nothing
		}
	}
}
