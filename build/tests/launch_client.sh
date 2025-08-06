ns1="230-237_samertt_CUBIC";

num_path=8
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

#-o "./${ns1}/${num_path}_path/$(date +'%Y-%m-%d_%H-%M-%S').log"

mkdir -p "./${ns1}/${num_path}_path"
./test_client > "./${ns1}/${num_path}_path/c_$(date +'%Y-%m-%d_%H-%M-%S')_${ns1}_${size_s}.output" -M -t 5 -c c -a 10.1.1.1 -p 8443 -s 100 -n 300 -l e  -i enp1s0f0 -i enp1s0f1  -i enp1s0f2 -i enp1s0f3  -i enp7s0f0 -i enp7s0f1  -i enp7s0f2 -i enp7s0f3 
echo "${size_s} test completed for ${ns1} with ${num_path} paths"