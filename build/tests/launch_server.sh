ns1="230-237_samertt_CUBIC";


tbsend=10000
case $tbsend in
  200)
    size_s="200KB";
    size_n=200000;
    ;;
  400)
    size_s="400KB";
    size_n=400000;
    ;;
  600)
    size_s="600KB";
    size_n=600000;
    ;;
  800)
    size_s="800KB";
    size_n=800000;
    ;;
  1000)
    size_s="1MB";
    size_n=1000000;
    ;;
  2000)
    size_s="2MB";
    size_n=2000000;
    ;;
  4000)
    size_s="4MB";
    size_n=4000000;
    ;;
  6000)
    size_s="6MB";
    size_n=6000000;
    ;;
  8000)
    size_s="8MB";
    size_n=8000000;
    ;;
  *)
    size_s="10MB";
    size_n=10000000;
    
esac

./test_server -l e  -M -c c -a 10.1.1.1  -p 8443 -s ${size_n}} # -l e #-o "./log/s_$(date +'%Y-%m-%d_%H-%M-%S')_${ns1}_${size_s}.log" #-w "./recieved.txt"

echo "${size_s} test completed for ${ns1}."