#ifndef SJTU_QUEUE_HPP
#define SJTU_QUEUE_HPP
namespace stl
{
	template<class T>
	class queue
	{
	private:
		struct node
		{
			T data;
			node *next;
			node(const T &x, node *N = NULL) : data(x), next(N) {}
			~node() {}
		};
		node *first, *last;
		int Size;
	public:
		queue()
		{
			first = last = NULL;
			Size = 0;
		}
		~queue()
		{
			clear();
		}
		bool empty()
		{
			return first == NULL;
		}
		void clear()
		{
			node *tmp;
			while (first != NULL) {
				tmp = first;
				first = first->next;
				delete tmp;
			}
			Size = 0;
		}
		T front()
		{
			return first->data;
		}
		void push(const T &x)
		{
			if (last == NULL)
				first = last = new node(x);
			else
				last = last->next = new node(x);
		}
		T pop()
		{
			node *tmp = first;
			T value = first->data;
			first = first->next;
			if (first == NULL)
				last = NULL;
			delete tmp;
			return value;
		}
	};
}
#endif