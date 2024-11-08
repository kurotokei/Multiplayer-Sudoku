
async function create_game() {

	
	let game_type = document.querySelector('input[name="game_type_select"]:checked').value
	let game_duration = Number(
							document.querySelector(
								'input[name="game_duration_select"]:checked'
							).value
						)

	let game_level = Number(
							document.querySelector(
								'input[name="game_level_select"]:checked'
							).value
						)



	params = {
				"game_type" : game_type,
				"game_duration" : game_duration,
				"game_level" : game_level
			};

	try {
		let response = await fetch("/api/create_game/",
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

		let raw_out = await response.text();

		let out = JSON.parse(raw_out);

		if( out["game_id"] != null )
			window.location.href = `/game/${out["game_id"]}`;
		else
		{
			waiting_for_match = true;
			set_divs_visibility();
		}
	}
	catch(err){
		console.log(JSON.stringify(err));
		document.getElementById("create_game_error").innerText = "An error occured while trying to create the game";
	}
}

function set_divs_visibility()
{
	let divs_id = [
					"logged_out__div",
					"create_game__div",
					"wait_for_match__div",
					"go_to_game__div"
				];

	// hide all of them
	for( let i = 0 ; i < divs_id.length ; i++ )
	{
		let div = document.getElementById(divs_id[i]);
		div.style.display = "none";
		div.style.visibility = "hidden";
	}

	let visible_div_id;

	if( username == null )
		visible_div_id = "logged_out__div";
	else if( current_game_id != null )
		visible_div_id = "go_to_game__div";
	else if( waiting_for_match )
		visible_div_id = "wait_for_match__div";
	else
		visible_div_id = "create_game__div";
	
	let visible_div = document.getElementById(visible_div_id);

	visible_div.style.display = "block";
	visible_div.style.visibility = "visible";
}

function init()
{
	set_divs_visibility();

	if( current_game_id != null )
		document.getElementById("current_game_link").href = `/game/${current_game_id}`;

}


async function cancel_match_search()
{
	await fetch("/api/cancel_match_search/",
			{
				method: "POST",
				credentials: 'same-origin',
				body: JSON.stringify({}),
				headers: {
						"X-CSRFToken": getCookie("csrftoken"),
						"Accept": "application/json",
						'Content-Type': 'application/json'
				}
			});
	
	waiting_for_match = false;

	set_divs_visibility();
}
