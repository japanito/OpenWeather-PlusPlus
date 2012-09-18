$(document).ready(function(){
	var hiddenCloud = $('<img id="hiddenCloud2" src="images/cloud-hidden2.png"></img>').css({'opacity' : 0});
	
	setTimeout(function(){
		$('.container-fluid').append(hiddenCloud);
		hiddenCloud.addClass('animated fadeInLeft');
	}, 4000);
	
});