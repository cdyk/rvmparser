#include <cassert>
#include <chrono>
#include "Store.h"

namespace {

  struct QueueItem
  {
    Geometry* from;
    Connection* connection;
    Vec3f up;
  };


  struct Context
  {
    Buffer<QueueItem> queue;
    unsigned front = 0;
    unsigned back = 0;
    unsigned connectedComponents = 0;
    unsigned connections = 0;

  };

  void enqueue(Context& context, Geometry* from, Connection* connection)
  {
    connection->temp = 1;

    assert(context.back < context.connections);
    context.queue[context.back].from = from;
    context.queue[context.back].connection = connection;

    context.back++;
  }

  void enqueueConnections(Context& context, Geometry* geo)
  {
    for (unsigned k = 0; k < 6; k++) {
      if (geo->connections[k] && geo->connections[k]->temp == 0) {
        enqueue(context, geo, geo->connections[k]);
      }
    }
  }

  void processItem(Context& context)
  {
    auto & item = context.queue[context.front++];

    for (unsigned i = 0; i < 2; i++) {
      if (item.from != item.connection->geo[i]) {
        auto * geo = item.connection->geo[i];

        enqueueConnections(context, geo);
      }
    }
  }

}

void align(Store* store, Logger logger)
{
  Context context;
  auto time0 = std::chrono::high_resolution_clock::now();
  for (auto * connection = store->getFirstConnection(); connection != nullptr; connection = connection->next) {
    connection->temp = 0;
    context.connections++;
  }


  context.queue.accommodate(context.connections);
  for (auto * connection = store->getFirstConnection(); connection != nullptr; connection = connection->next) {
    if (connection->temp) continue;

    context.front = 0;
    context.back = 0;
    enqueue(context, nullptr, connection);
    context.connectedComponents++;
    do {
      processItem(context);
    } while (context.front < context.back);
  }
  auto time1 = std::chrono::high_resolution_clock::now();
  auto e0 = std::chrono::duration_cast<std::chrono::milliseconds>((time1 - time0)).count();

  logger(0, "%d connected components in %d connections (%lldms).", context.connectedComponents, context.connections, e0);
}