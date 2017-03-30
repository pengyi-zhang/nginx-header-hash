# nginx-header-hash

C module for NGINX. This module should take a set of HTTP headers, and create an ID hash based on those headers. It should then print the hash and the headers used to make the hash on a page.

#### Spec:
* Headers choosen for hashing: host, user-agent
* Hashing function: Bernstein hash function
* Functionality:
  1. Redirect request header into resp body generation function.
  2. Extract headers of interest and re-construct hash_key string in JSON-like format.
  3. hash the key and write both hash_key and hash_val into content body buffer.
  4. Send back header and body.

#### Configure the module

1. Add module to Nginx
   ./configure --add-module={header-hash-module-path}
   make
   
2. Add followed content into nginx.conf to enable "header-hash" module when user browses to http://server_ip/distil_assesment

   `  
   location = /distil_assignment{  
     header_hash;  
   }
   `
    
   
