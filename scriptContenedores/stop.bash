echo "Stopping all running containers..."
sudo docker ps -q | xargs -r sudo docker stop

