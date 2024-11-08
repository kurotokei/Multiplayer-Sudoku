
let show_solution = false;
let show_solution_fg = "yellow";
let show_solution_bg = "green";


GRID_VALUE_ORIGIN_START = 0;
GRID_VALUE_ORIGIN_PLAYER_1 = 1;
GRID_VALUE_ORIGIN_PLAYER_2 = 2;

origin_to_fg = [ "black", "black", "black" ];
origin_to_bg = [ "white", "blue", "red" ];


let current_position = null;

let n = 0;
let canvas = null;
let ctx = null;

// currently selected button
// 0 means erase
let selected_grid_button = 0;

// sizes of different lines
// proportional to total data
let thin_size = 0.01
let bold_size = thin_size * 2

// set in init
let square_size = null;


/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////

function show_solution_button_function()
{
	let	elt = document.getElementById("show_solution_button");

	show_solution ^= 1;

	redraw_grid();

	if( show_solution )
		elt.innerText = "Hide solution";
	else
		elt.innerText = "Show solution";
}



function get_player_score(player_in_game_id)
{
	if( current_position == null )
		return null;

	let origin_code;
    if( player_in_game_id == 1 )
        origin_code = GRID_VALUE_ORIGIN_PLAYER_1;
    else
        origin_code = GRID_VALUE_ORIGIN_PLAYER_2;

	let score = 0;
	for( let row = 0 ; row < 9 ; row++ )
		for( let col = 0 ; col < 9 ; col++ )
			if( current_position["origins"][row][col] == origin_code &&
				(
					current_position["solution"] == undefined ||

					current_position["position"][row][col] ==
					current_position["solution"][row][col]
				)
			)
				score += 1;
	

	return score;
}


// used to set/update the player information
function reset_player_info(player_in_game_id)
{
	let elt = document.getElementById(`player_${player_in_game_id}_info`);

	let player_obj;

	if( player_in_game_id == 1 )
		player_obj = player_1;
	else
		player_obj = player_2;

	if( player_obj == null )
	{
		elt.innerText = `player_${player_in_game_id} : unknown`;
	}
	else
	{
		elt.innerText = `player_${player_in_game_id} : ${player_obj["name"]} ${Math.round(player_obj["rating"])}`;

		if( game["start_date"] != null )
		{
			elt.innerText += `, score = ${get_player_score(player_in_game_id)}`;

			if( game["end_date"] == null &&
				player_in_game_id == in_game_id 
			)
				elt.innerText += '?';
		}
	}


}

function reset_players_info()
{
	reset_player_info(1);
	if( game["type"] == "GAME_TYPE_SOLO" )
		hide_element("player_2_info");
	else
		reset_player_info(2);

}

function reset_game_result()
{
	if( game["type"] == "GAME_TYPE_SOLO" || 
		game["start_date"] == null ||
		game["end_date"] == null )
	{
		hide_element("game_result");
		return;
	}

	let elt = document.getElementById("game_result");
	elt.style.display = "block";
	elt.style.visibility = "visible";

	if( game["winner"] == null )
		elt.innerText = "draw";
	else if( game["winner"] == 1 )
		elt.innerText = "the winner is " + player_1.name;
	else
		elt.innerText = "the winner is " + player_2.name;

}



function init() {
	canvas = document.getElementById('can');
	ctx = canvas.getContext("2d");

	square_size = (1-( 4*bold_size + 6*thin_size ))/9;

	canvas.addEventListener('click', on_grid_click, false);

	reset_players_info();
	reset_game_result();

	game_socket = new WebSocket(
		'ws://'
		+ window.location.host
		+ '/ws/game/'
		+ game["id"]+'/'
	);

	game_socket.onmessage = game_socket__onmessage;
	game_socket.onclose   = game_socket__onclose;


	// hide the start button
	if( game["start_date"] != null || game["end_date"] != null )
		hide_element("start_game_button");

	if( game["end_date"] != null )
		hide_element("end_game_button");
	
	if( game["end_date"] == null )
		hide_element("show_solution_button");

}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////


// return the top left corner coord of the square
// at position row,col
// indexes start from 0
function get_square_coord(row, col)
{
	// same equation for both , from the x perspective :
	//	  left bold line
	//	  1 + ( x // 3 ) bold lines
	//	  x - x//3   thin lines

	let square_x,row_3;
	let square_y,col_3;

	row_3 = Math.floor(row/3);
	col_3 = Math.floor(col/3);

	square_y = ( 1 + row_3 ) * bold_size + ( row - row_3 ) * thin_size + square_size * row;
	square_x = ( 1 + col_3 ) * bold_size + ( col - col_3 ) * thin_size + square_size * col;

	return [square_x, square_y]
} // get_square_coord

function write_number(num, x, y, color)
{
	ctx.fillStyle = color;
	let [cx,cy] = get_square_coord(x,y);

	ctx.font = Math.floor(canvas.width*square_size)+"px Arial";

	// i found the the coefs by testing.
	// using textallign center did not work
	ctx.fillText(num,
				 canvas.width  * (cx+square_size*0.25),
				 canvas.height * (cy+square_size*0.9));

}

function color_square(x, y, color)
{
	ctx.fillStyle = color;
	let [cx,cy] = get_square_coord(x,y);

	ctx.fillRect(canvas.width  * cx,
				 canvas.height * cy,
				 canvas.width  * square_size,
				 canvas.height * square_size);
}

// draw both a vertical and a horizontal line
function draw_grid_line(line_start, line_width) {

	ctx.beginPath();
	ctx.moveTo( line_start * canvas.width, 0);
	ctx.lineTo( line_start * canvas.width, canvas.height);
	ctx.lineWidth = line_width * canvas.width;
	ctx.stroke();

	ctx.beginPath();
	ctx.moveTo( 0, line_start * canvas.height);
	ctx.lineTo( canvas.width, line_start * canvas.height);
	ctx.lineWidth = line_width * canvas.height;
	ctx.stroke();
}

function draw_grid_lines()
{
	// draw vertical lines
	// records end position of the last line
	let last_end = 0;
	for( let i = 0 ; i < 10 ; i++ )
	{
		if( i == 0 )
		{
			draw_grid_line(bold_size / 2, bold_size);
			last_end += bold_size;
		}
		else if( i % 3 == 0 )
		{
			start = last_end+square_size+bold_size/2;
			draw_grid_line(start, bold_size);
			last_end = start + bold_size / 2;
		}
		else
		{
			start = last_end+square_size+thin_size/2;
			draw_grid_line(start, thin_size);
			last_end = start + thin_size / 2;
		}
	}

} // draw_grid_lines

function redraw_grid()
{
	// clear the grid
	ctx.clearRect(0, 0, canvas.width, canvas.height);

	draw_grid_lines();

	if( current_position == null )
		return;
	
	for( let row = 0 ; row < 9 ; row++ )
		for( let col = 0 ; col < 9 ; col++ )
		{
			let val = current_position["position"][row][col];

			let origin = current_position["origins"][row][col];

			if(	show_solution && 
				current_position["solution"] != undefined &&
				current_position["solution"][row][col] != val )
			{
				val = current_position["solution"][row][col];

				color_square(row, col, show_solution_bg);
				write_number(val, row, col, show_solution_fg);
			}
			else if( val != 0 )
			{
				color_square(row, col, origin_to_bg[origin]);
				write_number(val, row, col, origin_to_fg[origin]);
			}
		
		}
}

////////////////////////////////////////////////

// change the currently selected button
// 0 is for erase
function set_selected_grid_button(new_selected)
{
	document.getElementById("grid_button_"+selected_grid_button).classList.remove("grid_button_selected");

	selected_grid_button = new_selected;

	document.getElementById("grid_button_"+selected_grid_button).classList.add("grid_button_selected");
}



////////////////////////////////////////////////

// given a click event on a grid
// return the corresponsing square
function get_click_square(event)
{
	let rect = canvas.getBoundingClientRect();

	let click_x = event.offsetX/rect.width;
	let click_y = event.offsetY/rect.height;

	let row,col;

	for( row = 1 ; row < 9 ; row++ )
	{
		let [cx,cy] = get_square_coord(row, row);

		if( click_y < cy )
			break;
	}

	for( col = 1 ; col < 9 ; col++ )
	{
		let [cx,cy] = get_square_coord(col, col);

		if( click_x < cx )
			break;
	}

	return [row-1,col-1];
}

// for now just call write call num ater a click on
// the canvas. the real function on click will a lot more complex
//
function on_grid_click(event)
{

	// if current position is not 
	// initialized or game is over
	// nothing to do
	if( current_position == null || game["end_date"] != null )
		return;

	let [row,col] = get_click_square(event);

	// modifications are allowed
	// if the sqaure is no known
	// or if it was written by the user
	if( ! (
			current_position["origins"][row][col] == in_game_origin_code ||
		(
			current_position["origins"][row][col] == GRID_VALUE_ORIGIN_START &&
			current_position["position"][row][col] == 0
		))
		)
	{
		return;
	}


	if( selected_grid_button == 0 )
	{
		// if values was set by player
		// set it to unkown
		current_position["origins"] [row][col] = GRID_VALUE_ORIGIN_START;
		current_position["position"][row][col] = 0;
	}
	else
	{
		// if value is not known
		current_position["origins"] [row][col] = in_game_origin_code;
		current_position["position"][row][col] = selected_grid_button;
	}


	game_socket.send(JSON.stringify({
									"type":"make_move",
									"data": {
												"row": row,
												"col": col,
												"val":selected_grid_button
											}
									}));


	// for now just redraw everything for every change
	// it is done at client side so it does not matter
	reset_players_info();
	redraw_grid();
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////


// this is used to notify the backend
// that this user is ready to start
async function notify_ready_to_start() {

	try {
		let params = {};

		let response = await fetch("/api/notify_player_is_ready/",
						{
							method: "POST",
							credentials: 'same-origin',
							body: JSON.stringify(params),
							headers: {
								"X-CSRFToken": getCookie("csrftoken"),
								"Accept": "application/json",
								'Content-Type': 'application/json'
							}
						});
	}
	catch(err) {
		console.log(JSON.stringify(err));
	}

}


async function end_game() {

	try {
		let params = {};

		let response = await fetch("/api/end_game/",
						{
							method: "POST",
							credentials: 'same-origin',
							body: JSON.stringify(params),
							headers: {
								"X-CSRFToken": getCookie("csrftoken"),
								"Accept": "application/json",
								'Content-Type': 'application/json'
							}
						});
	}
	catch(err) {
		console.log(JSON.stringify(err));
	}
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////

var game_socket;

function game_socket__onmessage(e) {
	const data = JSON.parse(e.data);

	if( data["error"] != undefined )
	{
		// FIXME: handle errors
	}
	else if( data["type"] == "position" )
	{
		current_position = JSON.parse(data["position"]);
		game["start_date"] = data["start_date"];
		hide_element("start_game_button");

		compute_expected_end_date();
		start_timer();

		reset_players_info();
		redraw_grid();
	}
	else if( data["type"] == "game_over" )
	{
		game["end_date"] = data["end_date"];
		game["winner"] = data["winner"];

		player_1["rating"] = data["player_1_rating"];

		if( player_2 != null )
			player_2["rating"] = data["player_2_rating"];


		hide_element("end_game_button");
		hide_element("start_game_button");
		hide_element("timer");

		reset_players_info();
		reset_game_result();

		show_element("show_solution_button");
	}
	else if( data["type"] == "player_2" )
	{
		player_2 = JSON.parse(data["player_2"]);
		reset_player_info(2);
	}
}

function game_socket__onclose(e) {
	console.error('Chat socket closed unexpectedly');
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////

// call start_timer to start the timer.
// you can call the function multiple times.
// timer_is_running is used to know if its already started.
// expected_end_date is used to not recompute the value every time.

var expected_end_date = null;
var timer_is_running  = false;

// if expected_end_date is not known
// try to compute it
function compute_expected_end_date()
{
	if( expected_end_date != null  ||  game["start_date"] == null )
		return;

	let start_date  = new Date(game["start_date"]);
	let duration_ms = game["duration"]*60*1000;

	expected_end_date = new Date(start_date.getTime() + duration_ms);
}

// update the timer text
// and return number of miliseconds 
// until next change of value
//
// return null if timer should be stopped
function update_timer()
{
	if( expected_end_date == null )
		return null;

	let now = new Date();
	let time_left_ms = expected_end_date - now;

	if( time_left_ms < 0 )
	{
		game_socket.send(JSON.stringify({
									"type":null,
									}));

		return null;
	}

	time_left_s = Math.round(time_left_ms/1000);

	let sec = (time_left_s % 60).toString();
	if( sec.length == 1 )
		sec = '0'+sec;

	let min = Math.floor(time_left_s/60);

	document.getElementById("timer").innerText = `${min}:${sec}`;


	return time_left_ms % 1000;
}

async function start_timer()
{
	if( expected_end_date == null || timer_is_running )
		return;

	timer_is_running = true;

	while( game["end_date"] == null )
	{
		let time_to_sleep = update_timer();

		if( time_to_sleep == null )
			break;

		await sleep(time_to_sleep);
	}

	timer_is_running = false;
}

