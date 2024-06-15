import paramiko
import time
import os

# -------------------------------------------------------------------------------- #
# Build configuration

mian_file_name = "lab2_003.c"
output_file_name = mian_file_name.split(".")[0]
build_command = "gcc -pthread -lrt -O2 -pedantic -Werror -o" + output_file_name

local_subdir = "lab2/"
local_dir_path = os.getcwd() + "/" + local_subdir
remote_dir_path = "/home/ad.cdv.pl/kkaczm27/wspol"

# -------------------------------------------------------------------------------- #
# Remote connection settings

remote_host = "mars.edu.cdv.pl"
username = "kkaczmarek6@edu.cdv.pl"


# ssh_key = "D:/Documents/keys/mars_cdv/mars_cdv_priv.pem"
ssh_key = "/home/kijada/.ssh/cdv_mars_key"
password = ""

# -------------------------------------------------------------------------------- #

def main():
    ssh = create_ssh_connection()

    try: 
        send_file(ssh, local_dir_path, remote_dir_path)
        build_file(ssh, remote_dir_path, mian_file_name, build_command)
    except Exception as e:
        print("Error occurred during operation: \n {}".format(e))

    print("-"*40)

    ssh.close()
    

# -------------------------------------------------------------------------------- #

def create_ssh_connection():
    print("-"*40)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    print("Connecting to remote host: ", remote_host)
    
    if password != "":
        print("Loggging method: password")
        ssh.connect(remote_host, username=username, password=password)
    else:
        print("Loggging method: ssh key")
        ssh.connect(remote_host, username=username, key_filename=ssh_key)
    
    if ssh.get_transport().is_active():
        print("Connected successfully!")
    else:
        print("Connection failed!")
        exit(1)

    return ssh


# -------------------------------------------------------------------------------- #

def send_file(ssh, source_path, dest_path):
    print("-"*40)
    print("Sending file from: {} to {}".format(source_path, dest_path))

    if not os.path.exists(source_path):
        raise Exception("Sorce directory does not exist")

    files_to_send_list = os.listdir(source_path)
    if len(files_to_send_list) == 0:
        raise Exception("Source directory is empty")

    print("Found {} files to send".format(len(files_to_send_list)))

    sftp = ssh.open_sftp()
    for file_name in files_to_send_list:
        sftp_local_path = os.path.join(source_path, file_name).replace("\\", "/")
        sftp_remote_path = os.path.join(dest_path, file_name).replace("\\", "/")
        local_file_szie = os.path.getsize(sftp_local_path) / 1024
        print("Sending file: {: <32} to {: <32} \t size: {: <3.1f}kB".format(sftp_local_path, sftp_remote_path, local_file_szie))
        sftp.put(sftp_local_path, sftp_remote_path)
    
    sftp.close()

    print("Files sent successfully")


# -------------------------------------------------------------------------------- #

def build_file(ssh, remote_path, mian_file, build_cmd):
    print("-"*40)
    command = "cd {};" .format(remote_path)
    command += "{} {}" .format(build_cmd, mian_file)
    print("Build command: ", command)

    stdin, stdout, stderr = ssh.exec_command(command)
    error = stderr.read().decode()
    output = stdout.read().decode()
    
    while not stdout.channel.exit_status_ready():
        time.sleep(0.25)
    
    print("Build output: \n", output)

    if(error):
        raise Exception("Error occurred during build: \n {}".format(error))
    else:
        print("File built successfully")


# -------------------------------------------------------------------------------- #

if __name__ == "__main__":
    main()
