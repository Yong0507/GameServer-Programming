my_id = -1
num = 0
contact_player_id = -1

local clock = os.clock
function sleep(n)  -- seconds
  local t0 = clock()
  while clock() - t0 <= n do end
end

function set_npc_id(id)
	my_id = id
	num = 0
end

function event_player_move_notify (player_id, x, y)
	contact_player_id = player_id
	my_x = API_get_x_position(my_id) 
	my_y = API_get_y_position(my_id)
	if x == my_x then
		if y == my_y then
			API_send_chat_packet(player_id, my_id, "Hello")
			API_add_move_event(my_id)
			num = num + 1
		end
	end
end

function check_move_count()
	if num >= 3 then
		API_send_chat_packet(contact_player_id, my_id, "BYE")
		num = 0
	else
		API_add_move_event(my_id)
		num = num + 1
	end
end



