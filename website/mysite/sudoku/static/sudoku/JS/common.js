// this is copied from django docs
// it is needed for post methods
// see :
// 		https://www.appsloveworld.com/django/100/121/django-csrf-error-in-fetch-api-ajax

function getCookie(name) {
	var cookieValue = null;
	if (document.cookie && document.cookie !== '') {
		var cookies = document.cookie.split(';');
		for (var i = 0; i < cookies.length; i++) {
			var cookie = cookies[i].trim();

			// Does this cookie string begin with the name we want?
			if (cookie.substring(0, name.length + 1) === (name + '=')) {
				cookieValue = decodeURIComponent(cookie.substring(name.length + 1));
				break;
			}
		}
	}

	return cookieValue;
}


// https://stackoverflow.com/questions/951021/what-is-the-javascript-version-of-sleep
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}


function hide_element(id)
{
	let elt = document.getElementById(id);

	elt.style.display = "none";
	elt.style.visibility = "hidden";
}


function show_element(id)
{
	let elt = document.getElementById(id);

	elt.style.display = "block";
    elt.style.visibility = "visible";
}



/////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////////////////////////////////////

const user_socket = new WebSocket(
    'ws://'
    + window.location.host
    + '/ws/user/'
    );

user_socket.onmessage = function(e) {
    const data = JSON.parse(e.data);

    if( data["error"] != undefined )
    {
        // FIXME: handle errors
    }
	else if( data["type"] == "new_game" )
	{
		// redirect to the game
		window.location.href = `/game/${data["game_id"]}`;
	}
};

user_socket.onclose = function(e) {
    console.error('user socket closed unexpectedly');
};
