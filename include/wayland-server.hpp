/*
 * Copyright (c) 2017, Nils Christopher Brause
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WAYLAND_SERVER_HPP
#define WAYLAND_SERVER_HPP

#include <functional>
#include <list>
#include <memory>
#include <string>

#include <wayland-server-core.h>
#include <wayland-util.hpp>

/** \file */

namespace wayland
{
  namespace server
  {
    namespace detail
    {
      struct listener_t
      {
        wl_listener listener;
        void *user;
      };
    }

    class client_t;
    template <class resource> class global_t;
    class event_loop_t;
    class event_source_t;

    class display_t
    {
    private:
      struct data_t
      {
        std::function<void()> destroy;
        std::function<void(client_t&)> client_created;
        detail::listener_t destroy_listener;
        detail::listener_t client_created_listener;
        wayland::detail::any user_data;
        unsigned int counter;
      };

      wl_display *display;
      data_t *data;

      static void destroy_func(wl_listener *listener, void *data);
      static void client_created_func(wl_listener *listener, void *data);
      static data_t *wl_display_get_user_data(wl_display *display);

    protected:
      display_t(wl_display *c);
      void init();
      void fini();

      friend class client_t;

    public:
      /** Create Wayland display object.
       *
       * This creates the display object.
       */
      display_t();

      /** Destroy Wayland display object.
       *
       * This function emits the wl_display destroy signal, releases
       * all the sockets added to this display, free's all the globals associated
       * with this display, free's memory of additional shared memory formats and
       * destroy the display object.
       *
       * \sa display_t::on_destroy
       */
      ~display_t();
      display_t(const display_t &d);
      display_t &operator=(const display_t& p);
      bool operator==(const display_t &c) const;
      wl_display *c_ptr() const;
      wayland::detail::any &user_data();

      event_loop_t get_event_loop();

      /** Add a socket to Wayland display for the clients to connect.
       *
       * \param name Name of the Unix socket.
       * \return 0 if success. -1 if failed.
       *
       * This adds a Unix socket to Wayland display which can be used by clients to
       * connect to Wayland display.
       *
       * If "" is passed as name, then it would look for WAYLAND_DISPLAY env
       * variable for the socket name. If WAYLAND_DISPLAY is not set, then default
       * wayland-0 is used.
       *
       * The Unix socket will be created in the directory pointed to by environment
       * variable XDG_RUNTIME_DIR. If XDG_RUNTIME_DIR is not set, then this function
       * fails and returns -1.
       *
       * The length of socket path, i.e., the path set in XDG_RUNTIME_DIR and the
       * socket name, must not exceed the maximum length of a Unix socket path.
       * The function also fails if the user do not have write permission in the
       * XDG_RUNTIME_DIR path or if the socket name is already in use.
       */
      
      int add_socket(std::string name);
      std::string add_socket_auto();

      /**  Add a socket with an existing fd to Wayland display for the clients to connect.
       *
       * \param sock_fd The existing socket file descriptor to be used
       * \return 0 if success. -1 if failed.
       *
       * The existing socket fd must already be created, opened, and locked.
       * The fd must be properly set to CLOEXEC and bound to a socket file
       * with both bind() and listen() already called.
       */
      int add_socket_fd(int sock_fd);
      void terminate();
      void run();
      void flush_clients();

      /** Get the current serial number
       *
       * This function returns the most recent serial number, but does not
       * increment it.
       */
      uint32_t get_serial();

      /** Get the next serial number
       *
       * This function increments the display serial number and returns the
       * new value.
       */
      uint32_t next_serial();
      std::function<void()> &on_destroy();

      /** Registers a listener for the client connection signal.
       *  When a new client object is created, \a listener will
       *  be notified, carrying the new client_t object.
       */
      std::function<void(client_t&)> &on_client_created();
      client_t client_create(int fd);

      /** Get the list of currently connected clients
       *
       * \param display The display object
       *
       * This function returns the list of clients currently
       * connected to the display.
       */
      const std::list<client_t> get_client_list();

      /** Set a filter function for global objects
       *
       * \param filter  The global filter funtion.
       *
       * Set a filter for the display to advertise or hide global objects
       * to clients.
       * The set filter will be used during global advertisment to
       * determine whether a global object should be advertised to a
       * given client, and during global binding to determine whether
       * a given client should be allowed to bind to a global.
       *
       * Clients that try to bind to a global that was filtered out will
       * have an error raised.
       */
      /** Adds a new protocol logger.
       *
       * When a new protocol message arrives or is sent from the server
       * all the protocol logger functions will be called, carrying the
       * \a user_data pointer, the type of the message (request or
       * event) and the actual message.
       * The lifetime of the messages passed to the logger function ends
       * when they return so the messages cannot be stored and accessed
       * later.
       *
       * \a errno is set on error.
       *
       * \param func The function to call to log a new protocol message
       *
       * \return The protol logger object on success, NULL on failure.
       */
      // TODO: wl_display_set_global_filter, wl_display_add_protocol_logger
    };

    class resource_t;

    class client_t
    {
    private:
      struct data_t
      {
        wl_client *client;
        std::function<void()> destroy;
        std::function<void(resource_t&)> resource_created;
        detail::listener_t destroy_listener;
        detail::listener_t resource_created_listener;
        wayland::detail::any user_data;
        unsigned int counter;
        bool destroyed;
      };

      wl_client *client;
      data_t *data;

      client_t() = delete;
      static void destroy_func(wl_listener *listener, void *data);
      static void resource_created_func(wl_listener *listener, void *data);
      static enum wl_iterator_result resource_iterator(struct wl_resource *resource, void *data);
      data_t *wl_client_get_user_data(wl_client *client);

    protected:
      client_t(wl_client *c);
      void init();
      void fini();

      friend class display_t;
      friend class resource_t;
      template <class resource> friend class global_t;

    public:
      /** Create a client for the given file descriptor
       *
       * \param display The display object
       * \param fd The file descriptor for the socket to the client
       * \return The new client object or NULL on failure.
       *
       * Given a file descriptor corresponding to one end of a socket, this
       * function will create a wl_client struct and add the new client to
       * the compositors client list.  At that point, the client is
       * initialized and ready to run, as if the client had connected to the
       * servers listening socket.  When the client eventually sends
       * requests to the compositor, the wl_client argument to the request
       * handler will be the wl_client returned from this function.
       *
       * The other end of the socket can be passed to
       * wl_display_connect_to_fd() on the client side or used with the
       * WAYLAND_SOCKET environment variable on the client side.
       *
       * Listeners added with wl_display_add_client_created_listener() will
       * be notified by this function after the client is fully constructed.
       *
       * On failure this function sets errno accordingly and returns NULL.
       */
      client_t(display_t &display, int fd);
      ~client_t();
      client_t(const client_t &p);
      client_t &operator=(const client_t& p);
      bool operator==(const client_t &c) const;
      wl_client *c_ptr() const;
      wayland::detail::any &user_data();
      void destroy();

      /** Flush pending events to the client
       *
       * Events sent to clients are queued in a buffer and written to the
       * socket later - typically when the compositor has handled all
       * requests and goes back to block in the event loop.  This function
       * flushes all queued up events for a client immediately.
       */
      void flush();

      /** Return Unix credentials for the client
       *
       * \param pid Returns the process ID
       * \param uid Returns the user ID
       * \param gid Returns the group ID
       *
       * This function returns the process ID, the user ID and the group ID
       * for the given client.  The credentials come from getsockopt() with
       * SO_PEERCRED, on the client socket fd.
       *
       * Be aware that for clients that a compositor forks and execs and
       * then connects using socketpair(), this function will return the
       * credentials for the compositor.  The credentials for the socketpair
       * are set at creation time in the compositor.
       */
      void get_credentials(pid_t &pid, uid_t &uid, gid_t &gid);

      /** Get the file descriptor for the client
       *
       * \return The file descriptor to use for the connection
       *
       * This function returns the file descriptor for the given client.
       *
       * Be sure to use the file descriptor from the client for inspection only.
       * If the caller does anything to the file descriptor that changes its state,
       * it will likely cause problems.
       *
       * See also client_t::get_credentials().
       * It is recommended that you evaluate whether client_t::get_credentials()
       * can be applied to your use case instead of this function.
       *
       * If you would like to distinguish just between the client and the compositor
       * itself from the client's request, it can be done by getting the client
       * credentials and by checking the PID of the client and the compositor's PID.
       * Regarding the case in which the socketpair() is being used, you need to be
       * careful. Please note the documentation for client_t::get_credentials().
       *
       * This function can be used for a compositor to validate a request from
       * a client if there are additional information provided from the client's
       * file descriptor. For instance, suppose you can get the security contexts
       * from the client's file descriptor. The compositor can validate the client's
       * request with the contexts and make a decision whether it permits or deny it.
       */
      int get_fd();
      std::function<void()> &on_destroy();

      /** Look up an object in the client name space
       *
       * \param id The object id
       * \return The object or NULL if there is not object for the given ID
       *
       * This looks up an object in the client object name space by its
       * object ID.
       */
      resource_t get_object(uint32_t id);
      void post_no_memory();

      /** Add a listener for the client's resource creation signal
       *
       * When a new resource is created for this client the listener
       * will be notified, carrying the new resource as the data argument.
       */
      std::function<void(resource_t&)> &on_resource_created();

      /** Get the display object for the given client
       *
       * \return The display object the client is associated with.
       */
      display_t get_display();
      std::list<resource_t> get_resource_list();
    };

    class resource_t
    {
    protected:
      // base class for event listener storage.
      struct events_base_t
      {
        virtual ~events_base_t() { }
      };

    private:
      struct data_t
      {
        wl_resource *resource;
        std::shared_ptr<events_base_t> events;
        std::function<void()> destroy;
        detail::listener_t destroy_listener;
        wayland::detail::any user_data;
        unsigned int counter;
        bool destroyed;
      };

      wl_resource *resource;
      data_t *data;

      static void destroy_func(wl_listener *listener, void *data);
      static int c_dispatcher(const void *implementation, void *target,
                              uint32_t opcode, const wl_message *message,
                              wl_argument *args);
      static int dummy_dispatcher(int opcode, std::vector<wayland::detail::any> args, std::shared_ptr<resource_t::events_base_t> events);

    protected:
      // Interface desctiption filled in by the each interface class
      static constexpr const wl_interface *interface = nullptr;

      /*
        Sets the dispatcher and its user data. User data must be an
        instance of a class derived from events_base_t, allocated with
        new. Will automatically be deleted upon destruction.
      */
      void set_events(std::shared_ptr<events_base_t> events,
                      int(*dispatcher)(int, std::vector<wayland::detail::any>, std::shared_ptr<resource_t::events_base_t>));

      // Retrieve the perviously set user data
      std::shared_ptr<events_base_t> get_events();

      void post_event_array(uint32_t opcode, std::vector<wayland::detail::argument_t> v);
      void queue_event_array(uint32_t opcode, std::vector<wayland::detail::argument_t> v);

      template <typename...T>
      void post_event(uint32_t opcode, T...args)
      {
        std::vector<wayland::detail::argument_t> v = { wayland::detail::argument_t(args)... };
        if(c_ptr())
          post_event_array(opcode, v);
      }

      template <typename...T>
      void queue_event(uint32_t opcode, T...args)
      {
        std::vector<wayland::detail::argument_t> v = { wayland::detail::argument_t(args)... };
        if(c_ptr())
          queue_event_array(opcode, v);
      }

      template <typename...T>
      void send_event(bool post, uint32_t opcode, T...args)
      {
        if(post)
          post_event(opcode, args...);
        else
          queue_event(opcode, args...);
      }

      void post_error(uint32_t code, std::string msg);

      resource_t(wl_resource *c);
      void init();
      void fini();

      friend class client_t;

    public:
      resource_t();

      /** Create a new resource object
       *
       * \param client The client owner of the new resource.
       * \param interface The interface of the new resource.
       * \param version The version of the new resource.
       * \param id The id of the new resource. If 0, an available id will be used.
       *
       * Listeners added with \a client_t::on_resource_created will be
       * notified at the end of this function.
       */
      resource_t(client_t client, const wl_interface *interface, int version, uint32_t id);
      ~resource_t();
      resource_t(const resource_t &p);
      resource_t &operator=(const resource_t& p);
      bool operator==(const resource_t &r) const;
      operator bool() const;
      wl_resource *c_ptr() const;
      wayland::detail::any &user_data();
      void destroy();

      void post_no_memory();
      uint32_t get_id();
      client_t get_client();
      unsigned int get_version() const;

      /** Retrieve the interface name (class) of a resource object.
       */
      std::string get_class();
      std::function<void()> &on_destroy();
    };

    template <class resource>
    class global_t
    {
    private:
      struct data_t
      {
        std::function<void(client_t, resource)> bind;
        wayland::detail::any user_data;
        unsigned int counter;
      };

      wl_global *global;
      data_t *data;

      global_t() = delete;

      static void bind_func(struct wl_client *cl, void *d, uint32_t ver, uint32_t id)
      {
        data_t *data = reinterpret_cast<data_t*>(d);
        client_t client(cl);
        resource res(client, ver, id);
        if(data->bind)
          data->bind(client, res);
      }

    protected:
      void fini()
      {
        data->counter--;
        if(data->counter == 0)
          {
            delete data;
            wl_global_destroy(c_ptr());
          }
      }

    public:
      global_t(display_t &display, unsigned int version = resource::max_version)
      {
        data = new data_t;
        data->counter = 1;
        global = wl_global_create(display.c_ptr(), resource::interface, version, data, bind_func);
      }

      ~global_t()
      {
        fini();
      }

      global_t(const global_t &g)
      {
        global = g.global;
        data = g.data;
        data->counter++;
      }

      global_t &operator=(const global_t& g)
      {
        fini();
        global = g.global;
        data = g.data;
        data->counter++;
        return *this;
      }

      bool operator==(const global_t &g) const
      {
        return c_ptr() == g.c_ptr();
      }

      wl_global *c_ptr() const
      {
        if(!global)
          throw std::runtime_error("global is null.");
        return global;
      }

      /* Check if a global filter is registered and use it if any.
       *
       * \param client The client on which the visibility shall be tested
       *
       * If no global filter has been registered, this funtion will
       * return true, allowing the global to be visible to the client
       */
      bool is_visible(client_t client)
      {
        return wl_global_is_visible(client.c_ptr(), c_ptr());
      }

      wayland::detail::any &user_data()
      {
        return data->user_data;
      }

      std::function<void(client_t, resource)> &on_bind()
      {
        return data->bind;
      }
    };

    class event_loop_t
    {
    private:
      struct data_t
      {
        std::function<void()> destroy;
        detail::listener_t destroy_listener;
        std::list<std::function<int(int, uint32_t)>> fd_funcs;
        std::list<std::function<int()>> timer_funcs;
        std::list<std::function<int(int)>> signal_funcs;
        std::list<std::function<void()>> idle_funcs;
        wayland::detail::any user_data;
        unsigned int counter;
      };

      wl_event_loop *event_loop;
      data_t *data;

      data_t *wl_event_loop_get_user_data(wl_event_loop *client);
      static void destroy_func(wl_listener *listener, void *data);
      static int event_loop_fd_func(int fd, uint32_t mask, void *data);
      static int event_loop_timer_func(void *data);
      static int event_loop_signal_func(int signal_number, void *data);
      static void event_loop_idle_func(void *data);

    protected:
      event_loop_t(wl_event_loop *p);
      void init();
      void fini();

      friend class display_t;

    public:
      event_loop_t();
      ~event_loop_t();
      event_loop_t(const event_loop_t &p);
      event_loop_t &operator=(const event_loop_t& p);
      bool operator==(const event_loop_t &p) const;
      wl_event_loop *c_ptr() const;
      wayland::detail::any &user_data();

      event_source_t add_fd(int fd, uint32_t mask, const std::function<int(int, uint32_t)> &func);
      event_source_t add_timer(const std::function<int()> &func);
      event_source_t add_signal(int signal_number, const std::function<int(int)> &func);
      event_source_t add_idle(const std::function<void()> &func);
      const std::function<void()> &on_destroy();
      int dispatch(int timeout);
      void dispatch_idle();
      int get_fd();
    };

    class event_source_t
    {
    private:
      wl_event_source *event_source;
      event_source_t() = delete;

    protected:
      event_source_t(wl_event_source *p);
      friend class event_loop_t;

    public:
      wl_event_loop *c_ptr() const;
      int timer_update(int ms_delay);
      int fd_update(uint32_t mask);
      void check();
    };
  }
}

#endif