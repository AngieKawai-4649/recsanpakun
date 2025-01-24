BEGIN{
	FS = "	"
}
{
#	if(NF != 5){
#		print NR "行目が5列になっていません"
#	}else{
		w = substr($1,0, 2)
		if( w == "BS" || w == "CS"){
			print "- name: " substr($5,index($5,";")+1, 20)
			print "  type: " w
			print "  channel: " $1
			print "  serviceId: : " $3
			print "  isDisabled: false\n"
		}
#	}
}
