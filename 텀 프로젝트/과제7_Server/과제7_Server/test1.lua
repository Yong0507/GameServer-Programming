myid = 9999

function set_uid(x)
	myid = x
end

function event_player_move(p_id)
	if(API_get_x(p_id) == API_get_x(myid)) then
		if API_get_y(p_id) == API_get_y(myid) then
			API_SendMessageHELLO(myid, player, "HELLO");
		end
	end
end

function event_say_bye(p_id)
	API_SendMessageBYE(myid, player, "BYE!");
end