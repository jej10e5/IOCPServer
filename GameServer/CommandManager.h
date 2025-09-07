#pragma once
#include "../ServerCommon/pch.h"
#include "Singleton.h"

class CommandManager : public Singleton<CommandManager>
{
public:
	CommandManager();
	~CommandManager();

	void Start()
	{
		string command;

		while (true)
		{
			cin >> command;

			if (command == "send")
			{

			}

		}
	}

};

